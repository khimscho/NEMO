/*! \file NVMFile.cpp
 *  \brief Generic Non-voltile Memory file storage for string data
 *
 * The logger has a number of instances where it needs to store one or more strings in
 * persistent, non-volatile, storage (typically flash memory of some kind).  This module
 * provides a primitive for this, and specialisations for particular cases.
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <set>
#include "NVMFile.h"
#include "SPIFFS.h"
#include "LogManager.h"
#include "serialisation.h"
#include "ArduinoJson.h"

namespace logger {

/// \class NVMFile
/// \brief Implementatin of code to read/write a non-volatile file
///
/// A number of modules in the firmware need to provide the facility of storing an arbitrary
/// number of strings in the non-volatile memory on the logger.  This class provides a means
/// to centralise that structure.  Note that this class is not generally exposed to the rest
/// of the code, which interacts via the interface classes to either read or write, but not
/// both together.

class NVMFile {
public:
    /// \enum FileMode
    /// \brief Determine the file mode to use on the backing file
    enum FileMode {
        READ,       ///< Read the contents
        WRITE,      ///< Write the contents from the start of the file
        APPEND      ///< Write on to the end of an existing file, or make a new file if necessary
    };

    /// \brief Default constructor, opening the file in the appropriate mode
    ///
    /// This opens the file specified in the appropriate mode.
    ///
    /// \param filename Backing filename to use for string storage
    /// \param mode     File read mode to use

    NVMFile(String const& filename, FileMode mode)
    : m_mode(mode)
    {
        m_source = new File(SPIFFS.open(filename.c_str(), mode == READ ? "r" : mode == WRITE ? "w" : "a"));
    }

    /// \brief Default destructor
    ///
    /// Close the backing file, and then delete the controller for it.

    ~NVMFile(void)
    {
        m_source->close();
        delete m_source;
    }

    /// \brief Read the next entry in the file
    ///
    /// This reads the next entry in the file, while is essentially a single line of the file.  Note
    /// that this only works in files opened for READ mode; otherwise and empty string is returned.
    ///
    /// \return Next line of the file, or the empty string if the file was not opened in READ mode.

    String ReadNext(void)
    {
        if (m_mode != READ)
            return String("");
        
        String rtn = m_source->readStringUntil('\n');
        rtn.trim();
        return rtn;
    }

    /// \brief Write a new entry into the file
    ///
    /// This writes the next line into the file.  If the file was opened in READ mode, this is a nullop.
    ///
    /// \param s    String to write into the file.

    void WriteNext(String const& s)
    {
        if (m_mode == READ)
            return;

        m_source->println(s);
    }

    /// \brief Returns the number of available bytes in the file
    ///
    /// This provides access to the available() method on the underlying file, allow the user to check
    /// whether the file has been completely read.  Files not opened in READ mode have undefined behaviour.
    ///
    /// \return Number of bytes still available to read in the underlying file.

    int Available(void)
    {
        return m_source->available();
    }

private:
    File        *m_source;  ///< Backing file to use for storage
    FileMode    m_mode;     ///< Mode used for accessing the file (READ, WRITE, APPEND)
};

/// This sets up for reading the specified file by instantiating the file in READ mode.
///
/// \param filename Name of the file to use for backing store

NVMFileReader::NVMFileReader(String const& filename)
{
    m_source = new NVMFile(filename, NVMFile::FileMode::READ);
}

NVMFileReader::~NVMFileReader(void)
{
    delete m_source;
}

/// Determine whether there are more entries in the file, as a binary flag.  Since any
/// entry in the file is a valid string, even a single byte left implies that there's
/// something else to read.
///
/// \return True if there's more entries to read, otherwise False

bool NVMFileReader::HasMore(void)
{
    if (m_source->Available())
        return true;
    else
        return false;
}

/// Extract the next entry from the backing file.  No translation is done to the file.
///
/// \return Next entry in the associated backing file (i.e., the next line as a string)

String NVMFileReader::NextEntry(void)
{
    return m_source->ReadNext();
}

/// Open the specified backing file for write, or append.
///
/// \param filename Name of the backing file to use for storage
/// \param append   Flag: True => add to the file, False => restart at the beginning

NVMFileWriter::NVMFileWriter(String const& filename, bool append)
{
    NVMFile::FileMode mode;
    if (append) {
        mode = NVMFile::FileMode::APPEND;
    } else {
        mode = NVMFile::FileMode::WRITE;
    }
    m_source = new NVMFile(filename, mode);
}

NVMFileWriter::~NVMFileWriter(void)
{
    delete m_source;
}

/// Add a new entry to the file.  Depending on the mode use, this could re-write the entry
/// or add to the end of the file.  Even if opened for write, multiple uses of this would
/// result in a file with multiple entries.
///
/// \param s    Entry to add to the file (i.e., next line)

void NVMFileWriter::AddEntry(String const& s)
{
    m_source->WriteNext(s);
}

/// Construct an interface to access the storage for auxiliary metadata associated with the
/// data being generated by the logger.

MetadataStore::MetadataStore(void)
{
    m_backingStore = String("/Metadata.txt");
}

/// Write the specified metadata into the associated backing store file.  Note that no test
/// or interpretation is done on the string: whatever you specify is what gets stored and then
/// dumped out with the data files.  Note that the string should contain no \r\n, otherwise there
/// will likely be problems getting the data back out.
///
/// \param s    Metadata string to store

void MetadataStore::WriteMetadata(String const& s)
{
    NVMFileWriter w(m_backingStore, false);
    w.AddEntry(s);
}

/// Extract the metadata from the logger's backing store, if it exists.
///
/// \return The stored metadata string

String MetadataStore::GetMetadata(void)
{
    NVMFileReader r(m_backingStore);
    return r.NextEntry();
}

/// Output metadata into an available binary output stream using the provided \a Serialiser object.
/// This allows the metadata to be added to any binary file being created.
///
/// \param s    Pointer to the \a Serialiser object to use to put the data into the output file

void MetadataStore::SerialiseMetadata(Serialiser *s)
{
    String meta = GetMetadata();
    Serialisable packet(meta.length() + 4);

    if (meta.length() > 0) {
        packet += meta.length();
        packet += meta.c_str();
        s->Process(logger::Manager::PacketIDs::Pkt_JSON, packet);
    }
}

ScalesStore::ScalesStore(void)
{
    m_backingStore = String("/scales.txt");
}

void ScalesStore::AddScalesGroup(String const& group, String const& scales)
{
    String  recog("\"" + group + "\"");
    String  temp_file("/scaletemp.txt");

    if (SPIFFS.exists(m_backingStore)) {
        NVMFileReader   in(m_backingStore);
        NVMFileWriter   temp(temp_file);
        String          entry;
        bool            group_out = false;

        while (in.HasMore()) {
            entry = in.NextEntry();
            if (entry.startsWith(recog)) {
                temp.AddEntry(recog + ":" + scales);
                group_out = true;
            } else {
                temp.AddEntry(entry);
            }
        }
        if (!group_out) {
            temp.AddEntry(recog + ":" + scales);
        }
    } else {
        NVMFileWriter   f(temp_file);
        f.AddEntry(recog + ":" + scales);
    }
    SPIFFS.remove(m_backingStore);
    SPIFFS.rename(temp_file, m_backingStore);
}

void ScalesStore::ClearScales(void)
{
    SPIFFS.remove(m_backingStore);
}

String ScalesStore::GetScales(void)
{
    NVMFileReader f(m_backingStore);
    String rtn = "{";
    String entry;
    bool first_out = false;

    while (f.HasMore()) {
        entry = f.NextEntry();
        if (first_out) {
            rtn += "," + entry;
        } else {
            first_out = true;
            rtn += entry;
        }
    }
    rtn += "}";
    return rtn;
}

/// Output the serial scales metadata to the provided binary output stream using the provided
/// \a Serialiser object.  This allows the scales packet to be added to any binary file being
/// created.
///
/// \param s    Pointer to the \a Serialiser object to use to put the data into the output file

void ScalesStore::SerialiseScales(Serialiser *s)
{
    String scales = GetScales();
    Serialisable packet(scales.length() + 4);

    if (scales.length() > 2) {
        packet += scales.length();
        packet += scales.c_str();
        s->Process(logger::Manager::PacketIDs::Pkt_SensorScales, packet);
    }
}

/// Start a new interface to the list of algorithm requests stored with the logger.  These are algorithms
/// that the logger would recommend be run on the data at the post-processing stage, although there is
/// no guarantee that this will occur.

AlgoRequestStore::AlgoRequestStore(void)
{
    m_algoBackingStore = "/algorithms.txt";
    m_paramBackingStore = "/parameters.txt";
}

/// Write a new algorithm into the backing store, optionally restarting the file in order to clear
/// out any mistakes or algorithms no longer required.  There is no way to edit the algorithms or
/// parameters in place other than to restart --- the logger isn't meant to be that smart, and all
/// of that complexity has to come from the user configuration.  Note that the code here also doesn't
/// check that the algorithm makes sense, or that the parameters are well formed --- it accepts
/// whatever the user provides.
///
/// \param alg_name     Name of the algorithm being requested
/// \param alg_params   Parameters associate with the algorithm being requested

void AlgoRequestStore::AddAlgorithm(String const& alg_name, String const& alg_params)
{
    NVMFileWriter alg(m_algoBackingStore);
    NVMFileWriter par(m_paramBackingStore);
    alg.AddEntry(alg_name);
    par.AddEntry(alg_params);
}

/// Empty the list of algorithms being stored by the logger, so that no requests are made of the
/// post-processing code.

void AlgoRequestStore::ResetList(void)
{
    NVMFileWriter names(m_algoBackingStore, false);
    NVMFileWriter params(m_paramBackingStore, false);
}

/// Write a list of the algorithms and parameters being recommended to the post-processing unit onto
/// an output stream (mainly for debugging and reference).
///
/// \param s    Anything Stream-like that supports the printf() iterface

void AlgoRequestStore::ListAlgorithms(Stream& s)
{
    NVMFileReader alg(m_algoBackingStore);
    NVMFileReader par(m_paramBackingStore);

    while (alg.HasMore()) {
        s.printf("alg = \"%s\", params = \"%s\"\n", alg.NextEntry().c_str(), par.NextEntry().c_str());
    }
}

// Convert the algorithm requests into a JSON structure that we can then convert to a \a String
// to send to the WiFi interface (or elsewhere)

DynamicJsonDocument AlgoRequestStore::MakeJSON(void)
{
    NVMFileReader alg(m_algoBackingStore);
    NVMFileReader par(m_paramBackingStore);

    DynamicJsonDocument algorithms(1024);

    int entry = 0;
    while (alg.HasMore()) {
        algorithms["algorithm"][entry]["name"] = alg.NextEntry();
        algorithms["algorithm"][entry]["parameters"] = par.NextEntry();
        ++entry;
    }
    algorithms["count"] = entry;
    return algorithms;
}

/// Write a set of output blocks into the binary WIBL-format file containing the algorithms and their
/// parameters that are being recommended to the post-processing code.
///
/// \param s    Pointer to the \a Serialiser to write data into the required ouptut file

void AlgoRequestStore::SerialiseAlgorithms(Serialiser *s)
{
    NVMFileReader alg(m_algoBackingStore);
    NVMFileReader par(m_paramBackingStore);
    
    while (alg.HasMore()) {
        Serialisable ser(255);
        String algorithm = alg.NextEntry();
        String parameters = par.NextEntry();
        ser += algorithm.length();
        ser += algorithm.c_str();
        ser += parameters.length();
        ser += parameters.c_str();
        s->Process(logger::Manager::PacketIDs::Pkt_Algorithms, ser);
    }
}

/// Instantiate a new interface to the list of NMEA0183 sentence message IDs that are expected to
/// be logged to the output data files on reception.

N0183IDStore::N0183IDStore(void)
{
    m_backingStore = "/NMEA0183IDs.txt";
}

/// Add a new identifier to the list of those allowed for logging at the NMEA0183 interfaces.  The
/// algorithm here just stores the information provided, and doesn't interpret it, but it's unlikely
/// to go well unless the strig contains a three-letter ID for a NMEA0183 message (e.g., "GGA", "RMC",
/// "GLL", "ZDA", "DBT", etc.)  No captialisation converion is done --- what you specify is what gets
/// checked.
///
/// \param msgid    String representation of the three-letter message ID

void N0183IDStore::AddID(String const& msgid)
{
    NVMFileWriter w(m_backingStore);
    w.AddEntry(msgid);
}

/// Reset the list of message IDs on the allowed list for logging; an empty list means "everything is
/// accepted".

void N0183IDStore::ResetFilter(void)
{
    NVMFileWriter w(m_backingStore, false);
}

/// Write the list of all of the message IDs in the filter to the provided Stream.
///
/// \param s    Anything Stream-like that supports the println() interface

void N0183IDStore::ListIDs(Stream& s)
{
    uint32_t n = 0;
    NVMFileReader r(m_backingStore);
    while (r.HasMore()) {
        s.printf("%d: %s\n", n, r.NextEntry().c_str());
        ++n;
    }
}

/// Generate a 

DynamicJsonDocument N0183IDStore::MakeJSON(void)
{
    NVMFileReader r(m_backingStore);
    DynamicJsonDocument messages(1024);

    int entry = 0;
    while (r.HasMore()) {
        messages["accepted"][entry] = r.NextEntry();
        ++entry;
    }
    messages["count"] = entry;
    return messages;
}

/// Write the list of all allowed message IDs to an output WIBL-format binary file using
/// the specified \a Serialiser.
///
/// \param s    Pointer to the \a Serialiser associated with the required output file

void N0183IDStore::SerialiseIDs(Serialiser *s)
{
    NVMFileReader r(m_backingStore);

    while (r.HasMore()) {
        Serialisable ser;
        String IDname = r.NextEntry();
        ser += IDname.length();
        ser += IDname.c_str();
        s->Process(logger::Manager::PacketIDs::Pkt_NMEA0183ID, ser);
    }
}

/// Extract a list of all of the message IDs allowed for logging into a set so that the
/// logging code can readily check whether a given sentence is allowed.  Note that the
/// provided output set is cleared before being set up.
///
/// \param s    Reference for the set to store the message IDs.

void N0183IDStore::BuildSet(std::set<String>& s)
{
    NVMFileReader r(m_backingStore);
    
    s.clear();
    while (r.HasMore()) {
        s.insert(r.NextEntry());
    }
}

}