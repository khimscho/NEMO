/*! \file NVMFile.h
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

#ifndef __NVMFILE_H__
#define __NVMFILE_H__

#include <set>
#include "Arduino.h"
#include "ArduinoJson.h"
#include "serialisation.h"

namespace logger {

// Forward declaration of base class, not required to be defined here
class NVMFile;

/// \class NVMFileReader
/// \brief Access class to read data from a NVMFile
///
/// Rather than deal with the NVMFile object that encapsulates the file, this class
/// provides a restricted interface that allows the user to just read the data in
/// the file.

class NVMFileReader {
public:
    /// \brief Default constructor, reading from the given file
    NVMFileReader(String const& filename);
    /// \brief Default destructor
    ~NVMFileReader(void);

    /// \brief Determine whether the file has more to read
    bool HasMore(void);
    /// \brief Extract the next String from the backing file
    String NextEntry(void);

private:
    NVMFile *m_source;  ///< Pointer to the backing file
};

/// \class NVMWriter
/// \brief Access class to write data to a NVMFile
///
/// Rather than deal with the NVMFile object that encapsulates the file, this class
/// provides a restricted interface that allow the user to just write the data in
/// the file.

class NVMFileWriter {
public:
    /// \brief Default constructor, writing to given file, optionally appending rather than clearing
    NVMFileWriter(String const& filename, bool append = true);
    /// \brief Default destructor
    ~NVMFileWriter(void);

    /// \brief Add a new line to the backing file
    void AddEntry(String const& s);

private:
    NVMFile *m_source;  ///< Pointer to the backing file
};

/// \class MetadataStore
/// \brief Specialisation of NVMFile for metadata storage
///
/// The logger can be used to store a JSON string with the platform-specific metadata
/// for the ship collecting the data.  This is written into the data files, allowing
/// the post-processing software to set up detailled, rather than general metadata
/// for the ship.
///     Note that the class does not attempt to check what's in the metadata, and just
/// stores what's given, and replicates it into the data.  What it should be, however,
/// is valid JSON in the format specified in IHO B-12 (Crowdsourced Bathymetry Working
/// Group guidelines) for the "platform" element, surrounded by a standard "{ ... }"
/// braces to make it a valid JSON document on its own (otherwise it can't be converted
/// into a Python dictionary using the standard JSON library).
///     Since the metadata string is stored as a single line in a file in non-volatile
/// memory, it's important that the JSON is all in one line, rather than being formatted
/// with spaces/tabs/carriage returns, etc.

class MetadataStore {
public:
    /// \brief Default constructor
    MetadataStore(void);

    /// \brief Store the specified string as the platform metadata element
    void WriteMetadata(String const& meta);
    /// \brief Return the whole metadata element string
    String GetMetadata(void);
    /// \brief Write the metadata element string into the output file associated with the \a Serialiser
    void SerialiseMetadata(Serialiser *ser);

private:
    String  m_backingStore; ///< Location of the file into which the metadata is stored
};

/// \class ScalesStore
/// \brief Specialisation of NVMFile for sensor scale factors
///
/// The logger can have multiple sensors that store their data in binary (or binary-coded) form,
/// needing scale factors applied before they become either usable floating-point data, or converted
/// into the right units.  This store allows the code to provide one or more JSON fragments of
/// scale factors to be stored for transmission into the output data files whenever a file is
/// started.

class ScalesStore {
public:
    /// \brief Default constructor.
    ScalesStore(void);

    /// \brief Store the specified scale factors as a SensorScales element
    void AddScalesGroup(String const& group, String const& scales);
    /// \brief Clear all scales groups from memory
    void ClearScales(void);
    /// \brief Return the whole scales JSON component
    String GetScales(void);
    /// \brief Serialise the scales JSON into the output file associated with the \a Serialiser provided
    void SerialiseScales(Serialiser *ser);

private:
    String m_backingStore; ///< Location of the file into which the scales are stored
};

/// \class AlgoRequestStore
/// \brief Specialisation of NVMFile for algorithm request storage
///
/// The logger can store a list of algorithms that should be applied to the data in post-processing
/// in order to correct any problems that might be generated by the specific circumstances of the
/// ship collecting the data.  There's no guarantee that the post-processing implementation will
/// have the algorithms, or that it will run them, but at least the request is made.
///     The algorithms are specified by a name and a parameter string.  There is no interpretation on
/// how the parameter string should be formatted, since only the algorithm needs to read it.

class AlgoRequestStore {
public:
    /// \brief Default constructor.
    AlgoRequestStore(void);

    /// \brief Add an algorithm to the list, optionally resetting the list
    void AddAlgorithm(String const& alg_name, String const& alg_params);
    /// \brief Reset the list of known algorithms
    void ResetList(void);
    /// \brief Generate a list of all requested algorithms to the output managed by \a Stream
    void ListAlgorithms(Stream& s);
    /// \brief Generate a JSON representation of the algorithms
    DynamicJsonDocument MakeJSON(void);
    /// \brief Write the list of all requested algorithms to the log file associated with \a Serialiser
    void SerialiseAlgorithms(Serialiser *s);

private:
    String  m_algoBackingStore;     ///< Name of the backing file for algorithm names
    String  m_paramBackingStore;    ///< Name of the backing file for algorithm parameters
};

/// \class N0183IDStore
/// \brief Specialisation of NVMFile for NMEA0183 allowed ID storage
///
/// In order to optimise the storage on the logger, the user can specify a list of the NMEA0183 message IDs
/// (the three-letter names that occur after the talked identifier in the messages) that should be written to
/// the data log files; all messages not in the list are ignored (except if there's an empty list, which
/// implicitly means that all messages should be accepted for logging).
///     The algorithm here does not check the message IDs provided for validity, case sensitivity, etc., and
/// filters against what the user provides.  The user is responsible for making sure that the names provided
/// actually make sense.

class N0183IDStore {
public:
    /// \brief Default constructor
    N0183IDStore(void);

    /// \brief Add another message ID to the accepted list
    void AddID(String const& msgid);
    /// \brief Reset the accepted list to empty
    void ResetFilter(void);
    /// \brief Provide a listing of all configured message IDs to the output \a Stream
    void ListIDs(Stream& s);
    /// \brief Write the list of accepted message IDs into the data log file associated with \a Serialiser
    void SerialiseIDs(Serialiser *s);
    /// \brief Make a JSON representation of the messageIDs that are being selected
    DynamicJsonDocument MakeJSON(void);
    /// \brief Read all of the message IDs and build the structure required for checking incoming messages
    void BuildSet(std::set<String>& s);

private:
    String  m_backingStore; ///< Location of the file into which the IDs for the NMEA messages allowed are stored
};

}

#endif
