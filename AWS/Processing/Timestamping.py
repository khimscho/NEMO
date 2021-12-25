# \file Timestamping.py
# \brief Read a binary data file from the SB2030 data logger, and generate timestamped data
#
# The SB2030 data logger reports, for each packet received, the elapsed time with respect
# to the logger's time ticker, and the data.  It also estimates the real time for the packet's
# reception time using whatever time information it has available, in real-time, using a forward
# extrapolation of the last time reference.  Of course, this isn't great, because it has to be
# causal.  This code is designed to get around that by reading all of the data to find the best
# time reference information, and then going back through again to provide timestamps for the
# packets based on all of the time reference data.
#    The code is pretty simple, just doing linear interpolation between timestamps of the reference
# time in order to generate the timestamps.  Reference time is, by preference NMEA2000 SystemTime,
# then the timestamps from NMEA2000 GNSS packets, and then finally from NMEA0183 GPGGA packets if
# there's nothing else available.
#
# Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
# Hydrographic Center, University of New Hampshire.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.

from os import stat
import numpy as np
import pynmea2 as nmea
import LoggerFile
import datetime as dt
from dataclasses import dataclass
from typing import Dict, List, Tuple, Any
from enum import Enum

## Exception to report that no adequate source of real-world time information is available
class NoTimeSource(Exception):
    pass

## Exception to indicate that there is no data to interpolate from the WIBL file
class NoData(Exception):
    pass

## Exception to indicate that the WIBL file is newer than the latest version understood by the code
class NewerDataFile(Exception):
    pass

## Construct an encoded integer representing a protocol version
#
# Create an encoded integer that represents a combined major/minor version
# number for a protocol.  This allows for simple integer comparison to determine
# newer/earlier/equivalent versions.
#
# \param major  Major version for the protocol implementation
# \param minor  Minor version for the protocol implementation
# \return Encoded integer for protocol version

def protocol_version(major: int, minor: int) -> int:
    """Convert the version information for the serialiser protocol to an integer so that we can do simple
       comparisons against what we find in the file being processed.

        Inputs:
            major   (int) Major verion number for a protocol implementaiton
            minor   (int) Minor version number for a protocol implementation
        Output:
            (int) Combined (encoded) version indicator
    """
    return major * 1000 + minor

# We need to make sure that we aren't asked to process a version of the data format newer than we know
# (although we should accept anything older).  These define the latest version of the protocol that's
# understood by the code.

## Latest major protocol version understood by the code
protocol_version_major = 1
## Latest minor protocol version understood by the code
protocol_version_minor = 1
 
## Maximum protocol version understood by the code
maximum_version = protocol_version(protocol_version_major, protocol_version_minor)

## \class StatCounters
#
# Provide a data object to count the number of times that a packet is observed in the data stream, and
# the count of faults observed when manipulating the packet (broken into a number of categories).  The
# object provides methods to count the total number of faults, and to serialise the contents for reporting.

@dataclass
class StatCounters:
    observed:       int = 0
    parse_fault:    int = 0
    short_msg:      int = 0
    decode_fault:   int = 0
    attrib_fault:   int = 0
    type_fault:     int = 0
    chksum_fault:   int = 0

    ## Count the number of times that the object has been observed in the datastream
    def Observed(self) -> None:
        self.observed += 1
    
    ## Count the number of times an object has failed to parse correctly
    def ParseFault(self) -> None:
        self.parse_fault += 1
    
    ## Count the number of times an object has come up short on the data expected
    def ShortMessage(self) -> None:
        self.short_msg += 1

    ## Count the number of times an object has failed to decode a bytes object into a string
    def DecodeFault(self) -> None:
        self.decode_fault += 1
    
    ## Count the number of times an object has thrown attribute errors during manipulation (usually a coding error)
    def AttributeFault(self) -> None:
        self.attrib_fault += 1
    
    ## Count the number of times an object has thrown type errors during manipulation (usually a coding error)
    def TypeFault(self) -> None:
        self.type_fault += 1
    
    ## Count the number of times an object has failed a checksum verification
    def ChecksumFault(self) -> None:
        self.chksum_fault += 1

    ## Count the total number of faults that have been seen on the object
    #
    # For reporting purposes, it's often a requirement to know how many faults have been
    # seen on the object (e.g., to determine whether to keep reporting errors).  This provides
    # the total number of faults recorded so far.
    #
    # \return Total number of faults recorded for this object

    def FaultCount(self) -> int:
        return self.parse_fault + self.short_msg + self.decode_fault + self.attrib_fault + self.type_fault + self.chksum_fault

    ## Generate a printable representation of the current object's information
    def __str__(self) -> str:
        total_fault = self.FaultCount()
        rtn = f'{self.observed:6} Obs.; Errors ({total_fault:6} total): {self.parse_fault:6} Parse / {self.short_msg:6} Short / {self.decode_fault:6} Decode / {self.attrib_fault:6} Attrib / {self.type_fault:6} Type / {self.chksum_fault:6} Checksum'
        return rtn

## Encapsulate the different types of faults that can be reported
#
# Enumeration for the different types of faults that can be tracked by the PktStats class
class PktFaults(Enum):
    ## A fault in parsing the packet for its individual components
    ParseFault = 0
    ## A packet that does not have enough data to adequately interpret
    ShortMessage = 1
    ## A fault when attempting to decode a packet (typically from bytes to str)
    DecodeFault = 2
    ## A fault in attributes when attempting to use a packet (usually a coding problem)
    AttributeFault = 3
    ## A fault in data types when attempting to use a packet (usually a coding problem)
    TypeFault = 4
    ## A fault in checksum verification for a packet
    ChecksumFault = 5

## Exception indicating that the code attempted to register an unknown fault type
class NoSuchFault(Exception):
    pass

## \class PktStats
#
# Provide a dictionary for statistics for an arbitrary set of packets to be tracked.  The object
# automatically adds new entries to the dictionary as any of the counting methods are called,
# although EnsureName() can be called to explicitly add the entry if required.  The Seen()
# method can be used to determine whether a particular packet has been observed in the data
# stream.  A constant can be specified at instantiation to provide a limit to the number of faults
# that can be seen and still report them (i.e., to avoid too many error messages from being
# issued on the output in verbose mode).  The object here does not mandate this: it just stores
# the constant on behalf of the user so that it doesn't need to get passed around the code.

class PktStats:
    ## Constructor for an empty dictionary
    #
    # This sets up the statistics tracker with a blank dictionary, and stores the reporting limit
    # on faults for later.
    #
    # \param fault_limit    Number of faults to report before suppressing output
    def __init__(self, fault_limit: int) -> None:
        self.fault_limit = fault_limit
        self.packets = {}
    
    ## Ensure that the packet specified is in the dictionary of objects being tracked
    #
    # Add the name object to the tracking list, with zeroed counters.  This is typically
    # not something that user-level code should call; rely on the code automatically adding
    # the packet name to the dictionary when you tell it either that you've seen it, or that
    # it caused a fault (Observed() or Fault() respectively).
    #
    # \param name   Name of the object to track
    def EnsureName(self, name: str) -> None:
        if name not in self.packets:
            self.packets[name] = StatCounters()

    ## Increment the count for how many times the named packet has been seen
    #
    # As a basic statistic, we count the number of times that each packet has been seen
    # in the data stream.  By default, this adds the packet name to the dictionary if
    # if has not been done before.
    #
    # \param name   Name of the object to track
    def Observed(self, name: str) -> None:
        self.EnsureName(name)
        self.packets[name].Observed()

    ## Increment the count for how many times a particular fault has been seen on the packet
    #
    # This allows the user to indicate that a fault has occurred in using the packet, and the
    # type of fault.  The statistics object tracks the count of each fault type separately.
    #
    # \param name   Name of the object that caused the fault
    # \param fault  (PktFaults) Fault that the packet caused
    def Fault(self, name: str, fault: PktFaults) -> None:
        self.EnsureName(name)
        if fault == PktFaults.ParseFault:
            self.packets[name].ParseFault()
        elif fault == PktFaults.ShortMessage:
            self.packets[name].ShortMessage()
        elif fault == PktFaults.DecodeFault:
            self.packets[name].DecodeFault()
        elif fault == PktFaults.AttributeFault:
            self.packets[name].AttributeFault()
        elif fault == PktFaults.TypeFault:
            self.packets[name].TypeFault()
        elif fault == PktFaults.ChecksumFault:
            self.packets[name].ChecksumFault()
        else:
            raise NoSuchFault()

    ## Determine whether the named packet has been seen in the data stream
    #
    # This checks whether the give name appears in the dictionary or not.  By proxy, this means
    # that the packet either was Observed(), or caused a Fault(), and therefore must have existed
    # in the data stream.
    #
    # \param name   Name of the object to check for
    # \return True if the name appears in the dictionary, otherwise False
    def Seen(self, name: str) -> bool:
        return name in self.packets

    ## Determine the total count of all packets that have been observed
    #
    # This computes the sum of all packets that have been Observed() in the dictionary.
    #
    # \return Count of all packets registered as seen in the data stream
    def TotalCount(self) -> int:
        rtn = 0
        for p in self.packets:
            rtn += self.packets[p].observed
        return rtn
    
    ## Determine the total count of faults registered for a given packet
    #
    # This computes the total number of faults of any kind registered for the particular
    # packet.
    #
    # \param name   Name of the object to report on
    # \return Total number of faults registered for the given packet
    def FaultCount(self, name: str) -> int:
        if name not in self.packets:
            raise NoSuchVariable()
        return self.packets[name].FaultCount()
    
    ## Generate a printable representation of the statistics for all of the packets observed
    def __str__(self) -> str:
        n_sentences = len(self.packets)
        rtn = f'Packet Statistics ({n_sentences} unique seen):\n'
        for p in self.packets:
            rtn += f'\t{p:>17}: {self.packets[p]}\n'
        return rtn

## Encapsulate the types of real-world time information that can be used
#
# In order to establish the relationship between the elapsed time (i.e., time of reception) of
# each packet and the real-world time, we need to use the encoded timestamps in one of the
# packets in the file.  There are a number of choices for this, depending on what type of
# data we have in there; this enumeration provides a controlled list of the options.
class TimeSource(Enum):
    ## Use NMEA2000 packet for SystemTime
    Time_SysTime = 0
    ## Use NMEA2000 packet from GNSS observations
    Time_GNSS = 1
    ## Use NMEA0183 ZDA timestamp packets
    Time_ZDA = 2
    ## Use NMEA0183 RMC minimum data with timestamps
    Time_RMC = 3

## Work out which time source in the WIBL file should be used for real time associations
#
# WIBL files have an elapsed time associated with each packet that tags the time of reception
# of the packet according to the local oscillator (which might not be reliable, but at least
# is monotonic).  There are multiple potential ways to assign a correspondence between these
# timestamps and a real-world time, which depend on the types of packets that we have in the
# file.  This code selects which of several sources is likely to be best.
#
# \param stats  (PktStats) Statistics on which packets have been seen in the data
# \return TImeSource enum for the source of real-world time to use for referencing

def determine_time_source(stats: PktStats) -> TimeSource:
    """Work out which source of time can be used to provide the translation between
       elapsed time (local time-stamps that indicate a monotonic clock tick at the
       logger when the packet is received) to a real world time.  The preference is
       to use NMEA2000 system time packets, but then to attempt to use GNSS packets,
       and then finally to drop back to NMEA0183 ZDA or RMC packets (in that order)
       if required.

        Inputs:
            stats   (PktStats) Statistics on which packets have been seen in the data
        
        Outputs:
            TimeSource enum for which source should be used for timestamping
    """
    if stats.Seen('SystemTime'):
        rtn = TimeSource.Time_SysTime
    elif stats.Seen('GNSS'):
        rtn = TimeSource.Time_GNSS
    elif stats.Seen('ZDA'):
        rtn = TimeSource.Time_ZDA
    elif stats.Seen('RMC'):
        rtn = TimeSource.Time_RMC
    else:
        raise NoTimeSource()
    return rtn

## Load the contents of a WIBL file and patch up any missing elapsed time entries
#
# We need to work over the data in a number of stages, so to avoid having to read the file
# more than once, we load all of the packets into memory first.  This allows the code to
# estimate statistics on how many packets have been read, and of what types (and if they have
# faults in their interpretation), and also to patch up any missing elapsed time entries on
# the packets (which can happen if another logger's data is translated into WIBL format for
# processing).
#
# \param filename       Filename to load for WIBL data
# \param verbose        Flag: set True to report more information on parsing.
# \param maxreports     Limit on how many errors should be reported before suppressing and summarising
# \return Tuple of PktStats, TimeSource, and a list of DataPacket entries from the file

def load_file(filename: str, verbose: bool, maxreports: int) -> Tuple[PktStats, TimeSource, List[LoggerFile.DataPacket]]:
    """Load the entirety of a WIBL binary file into memory, in the process determining the type of time
       source that can be used to add timestamps to the data, and fixing up any messages that don't have
       any elapsed time (i.e., time of reception) stamps.  This provides a set of data where it should
       be possible to continue processing without having to worry about missing information, although it
       does not guarantee that all of the data in the packets will be valid.

       Inputs:
            filename        Local filesystem name of the WIBL file to open and read
            verbose         Flag: set True to extra information on the process
            maxreports      Maximum number of errors per packet to report before summarising

        Outputs:
            stats           (PktStats) Statistics on which packets have been seen, and any problems
            time-source     (TimeSource) Indicator of which source of time should be used for real-time information
            packets         List[DataPacket] List if packets derived from the WIBL file
    """
    stats = PktStats(maxreports)
    packets = []
    needs_elapsed_time_fixup = False
    with open(filename, 'rb') as file:
        source = LoggerFile.PacketFactory(file)
        packet_count = 0
        while source.has_more():
            pkt = source.next_packet()
            if pkt is not None:
                if isinstance(pkt, LoggerFile.SerialString):
                    # We need to pull out the NMEA0183 recognition string
                    try:
                        name = pkt.data[3:6].decode('UTF-8')
                        stats.Observed(name)
                    except UnicodeDecodeError:
                        stats.Fault(name, PktFaults.DecodeFault)
                        continue
                else:
                    stats.Observed(pkt.name())
                packet_count += 1
                if pkt.elapsed == 0:
                    needs_elapsed_time_fixup = True
                if verbose and packet_count % 50000 == 0:
                    print(f'Reading file: passing {packet_count} packets ...')
                packets.append(pkt)
    
    # We need some form of connection from elapsed time stamps (i.e., when the packet is received
    # at the logger) and a real time, so that we can interpolate to real-time information for all
    # of the data.  There are a number of potential sources, including (in descending order of
    # preference), NMEA2000 SystemTime or GNSS, or NMEA0183 ZDA or RMC messages.  This code determines
    # which to use.
    timesource = determine_time_source(stats)

    # Under some circumstances, it is possible that packets will have no elapsed time in them
    # and therefore cannot be time-interpolated to a real time.  This generally is because the
    # source (i.e., from a non-WIBL logger) doesn't record elapsed times.  The best we can do in
    # this case is to fabricate an elapsed time based on whatever times we have, and then assign
    # an elapsed time to what's left based on the assumption that they had to have appeared at
    # some point between bordering timestamped data.  It's messy, but it's the best you're going
    # to get from loggers that don't record decent data ...
    if needs_elapsed_time_fixup:
        if timesource == TimeSource.Time_ZDA or timesource == TimeSource.Time_RMC:
            # If we're using NMEA0183 strings for timing, then we have the possibility of there being
            # packets where there's no elapsed time stamp, and we need to unpack the NMEA string
            # for its real-time stamp, and then assign an elapsed time based on this.  This sets us up for
            # generating intermediate elapsed time estimates subsequently.
            realtime_elapsed_zero = None
            for n in range(len(packets)):
                if isinstance(packets[n], LoggerFile.SerialString) and packets[n].elapsed == 0:
                    # Decode the packet string to identify ZDA/RMC
                    msg_id = packets[n].data[3:6].decode('UTF-8')
                    if (msg_id == 'ZDA' and timesource == TimeSource.Time_ZDA) or (msg_id == 'RMC' and timesource == TimeSource.Time_RMC):
                        if len(packets[n].data) < 11:
                            if verbose and stats.FaultCount(msg_id) < stats.fault_limit:
                                print(f'Error: short message {packets[n].data}; ignoring.')
                            stats.Fault(msg_id, PktFaults.ShortMessage)
                        else:
                            try:
                                msg = nmea.parse(packets[n].data.decode('UTF-8'))
                                if msg.datestamp is not None and msg.timestamp is not None:
                                    pkt_real_time = dt.datetime.combine(msg.datestamp, msg.timestamp)
                                    if realtime_elapsed_zero is None:
                                        realtime_elapsed_zero = pkt_real_time
                                        packets[n].elapsed = 0
                                    else:
                                        packets[n].elapsed = 1000.0*(pkt_real_time.timestamp() - realtime_elapsed_zero.timestamp())
                            except UnicodeDecodeError:
                                if verbose and stats.FaultCount(msg_id) < stats.fault_limit:
                                    print(f'Error: unicode decode failure on NMEA string; ignoring.')
                                stats.Fault(msg_id, PktFaults.DecodeFault)
                                continue
                            except nmea.ParseError:
                                if verbose and stats.FaultCount(msg_id) < stats.fault_limit:
                                    print(f'Error: parse error in NMEA string {packets[n].data}; ignoring.')
                                stats.Fault(msg_id, PktFaults.ParseFault)
                                continue
                            except TypeError:
                                if verbose and stats.FaultCount(msg_id) < stats.fault_limit:
                                    print(f'Error: type error unpacking NMEA string; ignoring.')
                                stats.Fault(msg_id, PktFaults.TypeFault)
                                continue
                    if stats.FaultCount(msg_id) >= stats.fault_limit:
                        print(f'Warning: too many errors on NMEA0183 packet {msg_id}; suppressing further reporting.')
        
        # We now need to patch up any packets that don't have an elapsed time using the mean of the
        # two nearest (ahead and behind) packets with known elapsed times.  The algorithm here finds
        # pairs of elapsed times, and then fills in all of the packets with no elapsed times between
        # them.
        oldest_position = None
        for n in range(len(packets)):
            if packets[n].elapsed == 0:
                if n > 0 and packets[n-1].elapsed > 0:
                    oldest_position = n - 1
            else:
                if oldest_position is not None:
                    if n - oldest_position > 1:
                        # This means we hit the end of a potential bracket, so we need to process the elapsed times
                        target_elapsed_time = (packets[oldest_position].elapsed + packets[n].elapsed)/2.0
                        for i in range(n - oldest_position - 1):
                            packets[oldest_position + 1 + i].elapsed = target_elapsed_time
                        oldest_position = None
        
        # Finally, we replace any timestamps that weren't interpolated to make sure that there's no question
        # that they're not valid.
        for n in range(len(packets)):
            #print(f'{n}: {packets[n].elapsed}')
            if packets[n].elapsed == 0:
               packets[n].elapsed = None

    return stats, timesource, packets

## Exception to indicate that the variable requested in the InterpTable does not exist
class NoSuchVariable(Exception):
    pass

## Exception to indicate that there are insufficient number of values in an add_points() call
class NotEnoughValues(Exception):
    pass

## Simple linear interpolation table for one or more dependent variables
#
# In order to establish real-world times for the packets, and their positions, we need to
# interpolate the asynchronous packets for this information against the elapsed times for
# the variables on interest.  This class provides facilities to register one or more named
# dependent variables (and automatically adds an independent variable), which can be added
# to as data becomes available.  Interpolation can then be done at given time points against
# one or more of the dependent variables, returned as a list of NumPy arrays.

class InterpTable:
    ## Constructor, specifying the names of the dependent variables to be interpolated
    #
    # This sets up for interpolation of an independent variable (added automatically) against
    # the named dependent variables.
    #
    # \param vars   (List[str]) List of the names of the dependent variables to manage
    def __init__(self, vars: List[str]) -> None:
        # Add an independent variable tag implicitly to the lookup table
        self.vars = {}
        self.vars['ind'] = []
        for v in vars:
            self.vars[v] = []
    
    ## Add a data point to a single dependent variable
    #
    # Add a single data point to a single dependent variable.  Note that if the object is
    # tracking more than one dependent variable, this is likely to cause a problem, since
    # you'll have one more point in the independent variable array than in the dependent
    # variables that aren't updated by this call, which will fail interpolation.  This is
    # therefore only really useful for the special case where there is only a single
    # dependent variable being tracked.
    #
    # \param ind    Independent variable value to add to the array
    # \param var    Name of the dependent variable to update
    # \param value  Value to add to the dependent variable array
    def add_point(self, ind: float, var: str, value: float) -> None:
        if var not in self.vars:
            raise NoSuchVariable()
        self.vars['ind'].append(ind)
        self.vars[var].append(value)
    
    ## Add a data point to multiple dependent variables simultaneously
    #
    # Add a data point for one or more dependent variables at a common independent variable
    # point.  Since adding a data point for a common independent variable value to some, but
    # not all of the dependent variables would result in some dependent variables having arrays
    # of different lengths, it would cause problems when interpolating.  This is therefore only
    # really useful if you're updating all of the variables.
    #
    # \param ind    Independent variable value to add to the array
    # \param vars   List of names of the dependent variables to update
    # \param values List of values to update for the named dependent variables, in the same order
    def add_points(self, ind: float, vars: List[str], values: List[float]) -> None:
        for var in vars:
            if var not in self.vars:
                raise NoSuchVariable()
        if len(vars) != len(values):
            raise NotEnoughValues()
        self.vars['ind'].append(ind)
        for n in range(len(vars)):
            self.vars[vars[n]].append(values[n])

    ## Interpolate one or more dependent variables at an array of independent variable values
    #
    # Construct a linear interpolation of the named dependent variables at the given array
    # of independent variable values.
    #
    # \param yvars  List of names of the dependent variables to interpolate
    # \param x      NumPy array of the independent variable points at which to interpolate
    # \return List of NumPy arrays for the named dependent variables, in the same order
    def interpolate(self, yvars: List[str], x: np.ndarray) -> List[np.ndarray]:
        for yvar in yvars:
            if yvar not in self.vars:
                raise NoSuchVariable()
        rtn = []
        for yvar in yvars:
            rtn.append(np.interp(x, self.vars['ind'], self.vars[yvar]))
        return rtn
    
    ## Determine the number of points in the independent variable array
    #
    # This provides the count of points that have been added to the table for interpolation.  Since
    # you have to add an independent variable point for each dependent variable point(s) added, this
    # should be the total number of points in the table.
    #
    # \return Number of points in the interpolation table
    def n_points(self) -> int:
        return len(self.vars['ind'])
    
    ## Accessor for the array of points for a named variable
    #
    # This provides checked access to one of the dependent variables stored in the array.  This returns
    # all of the points stored for that variable as a NumPy array.
    #
    # \param name   Name of the dependent variable to extract
    # \return NumPy array for the dependent variable named
    def var(self, name: str) -> np.ndarray:
        if name not in self.vars:
            raise NoSuchVariable()
        return self.vars[name]
    
    ## Accessor for the array of points for the independent variable
    #
    # This provides access to the independent variable array, without exposing the specifics
    # of how this is stored.
    #
    # \return NumPy array for the independent variable
    def ind(self) -> np.ndarray:
        return self.vars['ind']

## Construct a dictionary of interpolated observation data from a given WIBL file
#
# This carries out the basic read-convert-preprocess operations for a WIBL binary file, loading in all
# of the packets, fixing up any missing elapsed timestamps on the packets using real-world indicators
# in the packets, and then assigning real-world times to all of the recorded observable data packets,
# interpolating the positioning information at those points to give a (time, position, observation) set
# for each observation data set.  The data are encoded into a dictionary with auxiliary information
# such as metadata identifying the logger, required processing algorithms, etc.
#
# \param filename               Local filename for the source WIBL file
# \param elapsed_time_quantum   Maximum value that can be represented by the elapsed times in the packets
# \param verbose                Flag: set True to report lots more information as the processing proceeds
# \return Dictionary mapping identification names for the various datasets to the interpolated data arrays
def time_interpolation(filename: str, elapsed_time_quantum: int, verbose: bool) -> Dict[str, Any]:
    # Pull all of the packets out of the file, and fix up any preliminary problems
    stats, time_source, packets = load_file(filename, verbose, 10)
    if verbose:
        print(stats)

    time_table = InterpTable(['ref',])              # Mapping of elapsed time to real-world time
    position_table = InterpTable(['lon', 'lat'])    # Mapping of elapsed time to position information
    depth_table = InterpTable(['z',])               # Mapping of elapsed time to depth
    hdg_table = InterpTable(['h',])                 # Mapping of elapsed time to heading information
    wattemp_table = InterpTable(['temp',])          # Mapping of elapsed time to water temperature
    wind_table = InterpTable(['dir', 'spd'])        # Mapping of elapsed time to wind direction and speed

    logger_name = None      # Name of the logger (usually the identification)
    platform_name = None    # Name of the platform doing the logging
    metadata = None         # Any JSON metadata string provided in the WIBL file
    algorithms = []         # List of required processing algorithm name/parameter pairs provided in the WIBL file

    seconds_per_day = 24.0 * 60.0 * 60.0

    elapsed_offset = 0  # Estimate of the offset in milliseconds to add to elapsed times recorded (if we lap the counter)
    last_elapsed = 0    # Marker for the last observed elapsed time (to check for lapping the counter)

    stats = PktStats(10) # Reset statistics so that we don't double count on the second pass
    
    for pkt in packets:
        # There are some informational packets in the file that we can handle even if they
        # don't have assigned elapsed times; we deal with these first so that we can then
        # safely ignore any packets that are not time-enabled.
        if isinstance(pkt, LoggerFile.SerialiserVersion):
            stats.Observed(pkt.name())
            if protocol_version(pkt.major, pkt.minor) > maximum_version:
                raise NewerDataFile()
            logger_version = f'{pkt.major}.{pkt.minor}/{pkt.nmea2000_version}/{pkt.nmea0183_version}'
        if isinstance(pkt, LoggerFile.Metadata):
            stats.Observed(pkt.name())
            logger_name = pkt.logger_name
            platform_name = pkt.ship_name
        if isinstance(pkt, LoggerFile.JSONMetadata):
            stats.Observed(pkt.name())
            metadata = pkt.metadata_element.decode('UTF-8')
        if isinstance(pkt, LoggerFile.AlgorithmRequest):
            stats.Observed(pkt.name())
            algo = {
                "name": pkt.algorithm.decode('UTF-8'),
                "params": pkt.parameters.decode('UTF-8')
            }
            algorithms.append(algo)
        
        # After this point, any packet that we're interested in has to have an elapsed time assigned
        if pkt.elapsed is None:
            continue

        # We need to check that the elapsed ms counter hasn't wrapped around since
        # the last packet.  If so, we need to increment the elapsed time base stamp.
        # This method is very simple, and will fail mightily if the elapsed times in
        # the log file are not sequential and monotonic (modulo the elapsed_time_quantum).
        if pkt.elapsed < last_elapsed:
            elapsed_offset = elapsed_offset + elapsed_time_quantum
        last_elapsed = pkt.elapsed

        if isinstance(pkt, LoggerFile.SystemTime):
            stats.Observed(pkt.name())
            if time_source == TimeSource.Time_SysTime:
                time_table.add_point(pkt.elapsed + elapsed_offset, 'ref', pkt.date * seconds_per_day + pkt.timestamp)
        if isinstance(pkt, LoggerFile.Depth):
            stats.Observed(pkt.name())
            depth_table.add_point(pkt.elapsed + elapsed_offset, 'z', pkt.depth)
        if isinstance(pkt, LoggerFile.GNSS):
            stats.Observed(pkt.name())
            if time_source == TimeSource.Time_GNSS:
                time_table.add_point(pkt.elapsed + elapsed_offset, 'ref', pkt.date * seconds_per_day + pkt.timestamp)
            position_table.add_points(pkt.elapsed + elapsed_offset, ('lat', 'lon'), (pkt.latitude, pkt.longitude))
        if isinstance(pkt, LoggerFile.SerialString):
            try:
                pkt_name = pkt.data[3:6].decode('UTF-8')
                stats.Observed(pkt_name)
                data = pkt.data.decode('UTF-8')
                if stats.FaultCount(pkt_name) == stats.fault_limit:
                    print(f'Warning: too many errors on packet {pkt_name}; supressing further reporting.')
                if len(data) > 11:
                    try:
                        msg = nmea.parse(data)
                        if isinstance(msg, nmea.ZDA) and time_source == TimeSource.Time_ZDA:
                            if msg.datestamp is not None and msg.timestamp is not None:
                                reftime = dt.datetime.combine(msg.datestamp, msg.timestamp)
                                time_table.add_point(pkt.elapsed + elapsed_offset, 'ref', reftime.timestamp())
                        if isinstance(msg, nmea.RMC) and time_source == TimeSource.Time_RMC:
                            if msg.datestamp is not None and msg.timestamp is not None:
                                reftime = dt.datetime.combine(msg.datestamp, msg.timestamp)
                                time_table.add_point(pkt.elapsed + elapsed_offset, 'ref', reftime.timestamp())
                        if isinstance(msg, nmea.GGA) or isinstance(msg, nmea.GLL):
                            if msg.latitude is not None and msg.longitude is not None:
                                position_table.add_points(pkt.elapsed + elapsed_offset, ('lat', 'lon'), (msg.latitude, msg.longitude))
                        if isinstance(msg, nmea.DBT):
                            if msg.depth_meters is not None:
                                depth = float(msg.depth_meters)
                                depth_table.add_point(pkt.elapsed + elapsed_offset, 'z', depth)
                        if isinstance(msg, nmea.HDT):
                            if msg.heading is not None:
                                heading = float(msg.heading)
                                hdg_table.add_point(pkt.elapsed + elapsed_offset, 'h', heading)
                        if isinstance(msg, nmea.MWD):
                            if msg.direction_true is not None and msg.wind_speed_meters is not None:
                                direction = float(msg.direction_true)
                                speed = float(msg.wind_speed_meters)
                                wind_table.add_points(pkt.elapsed + elapsed_offset, ('dir', 'spd'), (direction, speed))
                        if isinstance(msg, nmea.MTW):
                            if msg.temperature is not None:
                                temp = float(msg.temperature)
                                wattemp_table.add_point(pkt.elapsed + elapsed_offset, 'temp', temp)
                    except nmea.ParseError as e:
                        if verbose and stats.FaultCount(pkt_name) < stats.fault_limit:
                            print(f'Parse error: {e}')
                        stats.Fault(pkt_name, PktFaults.ParseFault)
                        continue
                    except AttributeError as e:
                        if verbose and stats.FaultCount(pkt_name) < stats.fault_limit:
                            print(f'Attribute error: {e}')
                        stats.Fault(pkt_name, PktFaults.AttributeFault)
                        continue
                    except TypeError as e:
                        if verbose and stats.FaultCount(pkt_name) < stats.fault_limit:
                            print(f'Type error: {e}')
                        stats.Fault(pkt_name, PktFaults.TypeFault)
                        continue
                    except nmea.ChecksumError as e:
                        if verbose and stats.FaultCount(pkt_name) < stats.fault_limit:
                            print(f'Checksum error: {e}')
                        stats.Fault(pkt_name, PktFaults.ChecksumFault)
                        continue
                else:
                    # Packets have to be at least 11 characters to contain all of the mandatory elements.
                    # Usually a short packet is broken in some fashion, and should be ignored.
                    if verbose and stats.FaultCount(pkt_name) < stats.fault_limit:
                        print(f'Error: short message: {data}; ignoring.')
                    stats.Fault(pkt_name, PktFaults.ShortMessage)
            except UnicodeDecodeError as e:
                if verbose and stats.FaultCount(pkt_name) < stats.fault_limit:
                    print(f'Decode error: {e}')
                stats.Fault(pkt_name, PktFaults.DecodeFault)
                continue
    if verbose:
        print('Reference time table length = ', time_table.n_points())
        print('Position table length = ', position_table.n_points())
        print('Depth observations = ', depth_table.n_points())
        print(stats)

    if depth_table.n_points() < 1:
        raise NoData()
        
    # Finally, do the interpolations to generate the output data, and package it up as a dictionary for return
    depth_timepoints = depth_table.ind()
    z_times = time_table.interpolate(['ref'], depth_timepoints)[0]
    z_lat, z_lon = position_table.interpolate(['lat', 'lon'], depth_timepoints)
    
    hdg_timepoints = hdg_table.ind()
    hdg_times = time_table.interpolate(['ref'], hdg_timepoints)[0]
    hdg_lat, hdg_lon = position_table.interpolate(['lat', 'lon'], hdg_timepoints)
    
    wt_timepoints = wattemp_table.ind()
    wt_times = time_table.interpolate(['ref'], wt_timepoints)[0]
    wt_lat, wt_lon = position_table.interpolate(['lat', 'lon'], wt_timepoints)
    
    wind_timepoints = wind_table.ind()
    wind_times = time_table.interpolate(['ref'], wind_timepoints)[0]
    wind_lat, wind_lon  = position_table.interpolate(['lat', 'lon'], wind_timepoints)

    return {
        'loggername': logger_name,
        'platform': platform_name,
        'loggerversion': logger_version,
        'metadata': metadata,
        'algorithms': algorithms,
        'depth' : {
            't': z_times,
            'lat': z_lat,
            'lon': z_lon,
            'z': depth_table.var('z')
        },
        'heading' : {
            't': hdg_times,
            'lat': hdg_lat,
            'lon': hdg_lon,
            'heading': hdg_table.var('h')
        },
        'watertemp' : {
            't': wt_times,
            'lat': wt_lat,
            'lon': wt_lon,
            'temperature': wattemp_table.var('temp')
        },
        'wind' : {
            't': wind_times,
            'lat': wind_lat,
            'lon': wind_lon,
            'direction': wind_table.var('dir'),
            'speed': wind_table.var('spd')
        }
    }
