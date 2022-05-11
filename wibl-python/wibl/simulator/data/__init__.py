from typing import NamedTuple, NoReturn
from datetime import datetime, date, timedelta
import binascii
import logging

from wibl.simulator.data.writer import Writer


logger = logging.getLogger(__name__)


class Serialisable:
    pass


class TimeDatum:
    """
    POD for a time instant

    This provides a data object that represents a single point in time, with real-time interpolated
    from the elapsed time at construction based on the real-time reference and elapsed time
    offset in the supporting \a Timestamp object.
    """
    def __init__(self):
        self.datestamp: int = 0
        self.timestamp: float = -1.0
        self._m_elapsed: int = 0

    def is_valid(self) -> bool:
        """
        Test whether timestamp is valid
        :return:
        """
        return self.timestamp >= 0.0

    def serialise(self, target: Serialisable) -> NoReturn:
        """
        Serialise the datum into a given target
        :param target:
        :return:
        """
        pass

    def serialisation_size(self) -> int:
        """
        Give the size of the object once serialised
        :return: ?! Probably don't need this. !?
        """
        pass

    def printable(self) -> str:
        """
        Provide something that's a printable version
        :return:
        """
        pass

    def raw_elapsed(self) -> int:
        """
        Provide the raw observation (rarely required)
        :return:
        """
        return self._m_elapsed


class Timestamp:
    def __init__(self, date: int, timestamp: float, counter: int):
        self._m_last_datum_date: int = date
        self._m_last_datum_time: float = timestamp
        self._m_elapsed_time_at_datum: int = counter

    def update(self, date: int, timestamp: float, *, counter: int = None) -> NoReturn:
        """
        Provide a new observation of a known (UTC) time and (optionally) elapsed time
        :param date:
        :param timestamp:
        :param counter:
        :return:
        """
        pass

    def is_valid(self) -> bool:
        """
        Indicate whether a good timestamp has been generated yet
        :return:
        """
        return self._m_last_datum_time >= 0.0

    def now(self) -> TimeDatum:
        """
        Generate a timestamp for the current time, if possible.
        :return:
        """
        pass

    def datum(self) -> TimeDatum:
        """
        Generate a datum for the reference information
        :return:
        """
        pass

    def printable(self) -> str:
        """
        Generate a string-printable representation of the information
        :return:
        """
        pass

    def count_to_milliseconds(self, count: int) -> float:
        """
        Convert from internal count to milliseconds
        :param count:
        :return:
        """
        pass


class ComponentDateTime:
    """
    POD structure to maintain the broken-out components associated with a date-time

    The simulator has to be able to record a current time of the system state, which is modelled
    as a number of ticks of the system clock since the Unix epoch.  The class also allows for this
    information to be converted into a number of different formats as required by the NMEA0183 and
    NMEA2000 packets.
    """
    def __init__(self):
        """
        Empty constructor for an initialised (but invalid) object
        """
        # System clock ticks for the current time
        self.tick_count: int = None
        # Converted Gregorian year
        self.year: int = None
        # Day of the current time within the year
        self.day_of_year: int
        # Hour of the current time with the day-of-year
        self.hour: int
        # Minute of the current time within the hour
        self.minute: int
        # Second (with fractions) of the current time within the minute
        self.second: float

    def update(self, tick_count: int):
        """
        Set the current time (in clock ticks)
        :param tick_count:
        :return:
        """
        pass

    def days_since_epoch(self) -> int:
        """
        Compute and return the number of days since Unix epoch for the current time
        :return:
        """
        pass

    def seconds_in_day(self) -> float:
        """
        Compute and return the total number of seconds for the current time since midnight
        :return:
        """
        pass

    def time(self):
        """
        Convert the current time into a TimeDatum for use in data construction
        :return: ?! Timestamp::TimeDatum but maybe just Python datetime !?
        """


class State:
    def __init__(self):
        """
        State objects should only be constructed from within Engine
        """
        # Current timestamp for the simulation
        self._sim_time: datetime = None
        # Depth in metres
        self._current_depth: float = 10.0
        # Depth sounder measurement uncertainty, std. dev.
        self._measurement_uncertainty: float = 0.06
        # Reference timestamp for the ZDA/SystemTime
        self._ref_time: datetime = None
        # Longitude in degrees
        self._current_longitude: float = -75.0
        # Latitude in degress
        self._current_latitude: float = 43.0

        # Next clock tick count at which to update reference time state
        self._target_reference_time: int = None
        # Next clock tick count at which to update depth state
        self._target_depth_time: int = None
        # Next clock tick count at which to update position state
        self._target_position_time: int = None

        # Standard deviation, metres change in one second
        self._depth_random_walk: float = 0.02

        # Change in latitude/longitude per second
        self._position_step: float = 3.2708e-06
        # Current direction of latitude change
        self._latitude_scale: float = 1.0
        # Last time the latitude changed direction
        self._last_latitude_reversal: float = 0.0


class DataGenerator:
    """
    Encapsulate the requirements for generating NMEA data logged in binary format

    This provides a reference mechanism for generate NMEA messages from the simulator, and writing
    them into the output data file in the appropriate format.  This includes NMEA2000 and NMEA0183 data
    messages for the same position, time, and depths, so that the output data file is consistent with either
    data source.
    """
    def __init___(self, emit_nmea0183: bool = True, emit_nmea2000: bool = True):
        """
        Default constructor, given the NMEA2000 object that's doing the data capture
        :param emit_nmea0183:
        :param emit_nmea2000:
        :return:
        """
        # Flag for verbose debug output
        self._m_verbose: bool = False
        # Timestamp information for the current output state
        self._m_now: Timestamp = Timestamp(0, 0.0, 0)
        # Emit serial strings for NMEA0183
        self._m_serial: bool = emit_nmea0183
        # Emit binary data for NMEA2000
        self._m_binary: bool = emit_nmea2000

        if not emit_nmea0183 and not emit_nmea2000:
            logger.warning('User asked for neither NMEA0183 or NMEA2000; defaulting to generating NMEA2000')
            self._m_binary = True

    def emit_time(self, state: State, output: Writer) -> NoReturn:
        """
        Generate a timestamping message for the current state of the simulator
        :param state:
        :param writer:
        :return:
        """
        if self._m_binary:
            self._generate_system_time(state, output)
        if self._m_serial:
            self._generate_zda(state, output)

    def emit_position(self, state: State, output: Writer) -> NoReturn:
        """
        Generate messages for the current configuration of the simulator
        :param state:
        :param writer:
        :return:
        """
        if self._m_binary:
            self._generate_gnss(state, output)
        if self._m_serial:
            self._generate_gga(state, output)

    def emit_depth(self, state: State, output: Writer) -> NoReturn:
        """
        Generate a depth message from the current state of the simulator
        :param state:
        :param writer:
        :return:
        """
        if self._m_binary:
            self._generate_depth(state, output)
        if self._m_serial:
            self._generate_dbt(state, output)

    def set_verbose(self, verb: bool):
        """
        Set verbose logging state
        :param verb:
        :return:
        """
        self._m_verbose = verb

    def _generate_system_time(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA2000 timestamp information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_gnss(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA2000 positioning information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_depth(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA2000 depth information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_zda(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA0183 timestamp (ZDA) information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_gga(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA0183 positioning (GPGGA) information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_dbt(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA0183 depth (SDDBT) information
        :param state:
        :param output:
        :return:
        """
        pass

    def _compute_checksum(self, msg: str) -> int:
        """
        Compute a NMEA0183 sentence checksum
        :param msg:
        :return:
        """
        chk: int = 0
        for b in binascii.a2b_qp(msg[1:-1]):
            chk ^= b
        return chk

    class FormattedAngle(NamedTuple):
        """
        Break an angle in degrees into the components required to format for ouptut
        in a NMEA0183 GGA message (i.e., integer degrees and decimal minutes, with an
        indicator for which hemisphere to use).  To allow this to be uniform, the code
        assumes a positive angle is hemisphere 1, and negative angle hemisphere 0.  It's
        up to the caller to decide what decorations are used for those in the display.
        """
        # Reference to space for the integer part of the angle (degrees)
        degrees: int
        # Reference to space for the decimal minutes part of the angle
        minutes: float
        # Reference to space for a hemisphere indicator
        hemisphere: int

    def _format_angle(self, angle: float) -> FormattedAngle:
        """
        Convert an angle in decimal degrees into integer degrees and decimal minutes, with hemispehere indicator
        :param angle:
        :return: FormattedAngle
        """
        if angle > 0.0:
            out_hemi = 1
            out_angle = angle
        else:
            out_hemi = 0
            out_angle = -angle

        out_degrees = int(out_angle)
        out_minutes = out_angle - out_degrees

        return DataGenerator.FormattedAngle(degrees=out_degrees, minutes=out_minutes, hemisphere=out_hemi)

    class MonthDay(NamedTuple):
        month: int
        day: int

    def _to_day_month(self, year: int, year_day: int) -> MonthDay:
        """
        Convert the current time into broken out form
        :param year:
        :param year_day:
        :return: MonthDay
        """
        d = date(year, 1, 1) + timedelta(days=year_day - 1)
        return DataGenerator.MonthDay(d.month, d.day)


class Engine:
    """
    Run the core simulator for NMEA data output
    """
    def __init__(self, data_generator: DataGenerator):
        """
        Default constructor for a simulation engine
        """
        # current state information
        self._m_state = None
        # converter for state to output representation
        self._m_generator = data_generator

    def step_engine(self, write) -> int:
        """
        Move the state of the engine on to the next simulation time
        :param write:
        :return:
        """
        pass

    def _step_position(self, now: int) -> bool:
        """
        Step the position state to a given simulation time (if required)
        :param now:
        :return:
        """
        return False

    def _step_depth(self, now: int) -> bool:
        """
        Step the depth state to a given simulation time (if required)
        :param now:
        :return:
        """
        return False

    def _step_time(self, now: int) -> bool:
        """
        Step the real-world time state to a given simulation time (if required)
        :param now:
        :return:
        """
        return False
