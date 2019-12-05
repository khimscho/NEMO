/*!\file serialisation.h
 * \brief Simple serialisation of basic types for SD card storage
 *
 * As part of the logging process, we have to put together binary packets
 * of the input data for storage on the SD card, so that we economise on
 * the space taken per packet (and therefore get the most data per MB).
 * In order to provide some central capability for this, the Serialisable
 * type is used to encapsulate a packet of data to be written, and the
 * Serialiser class can be used to provide the overall encapsulation of
 * data type, length byte, checksum, etc. as required.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping
 * and NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#ifndef __SERIALISATION_H__
#define __SERIALISATION_H__

#include <stdint.h>
#include <SD.h>

const int SerialiserVersionMajor = 1; ///< Major version number for the serialiser
const int SerialiserVersionMinor = 0; ///< Minor version number for the serialiser

/// \class Serialisable
/// \brief Provide encapsulation for data to be written to store
///
/// This object provides an expandable buffer that can be used to hold data in preparation
/// for writing to SD card.  The buffer can be pre-allocated to a nominal size before starting
/// in order to avoid having to re-allocate the buffer during preparation (which is slow).

class Serialisable {
public:
    /// \brief Default constructor for the buffer object
    Serialisable(uint32_t size_hint = 255);
    /// \brief Default destructor
    ~Serialisable(void);
    
    /// \brief Add a single byte to the output buffer
    void operator+=(uint8_t b);
    /// \brief Add a 2-byte half-word to the output buffer
    void operator+=(uint16_t w);
    /// \brief Add a 4-byte word to the output buffer
    void operator+=(uint32_t dw);
    /// \brief Add an 8-byte double word to the output buffer
    void operator+=(uint64_t ul);
    /// \brief Add a single-precision floating point number to the ouptut buffer
    void operator+=(float f);
    /// \brief Add a double-precision floating point number of the output buffer
    void operator+=(double d);
    /// \brief Add an array of characters (C-style string) to the output buffer
    void operator+=(const char *p);
    
private:
    friend class Serialiser;
    uint8_t     *m_buffer;      ///< Pointer to the buffer being assembled
    uint32_t    m_bufferLength; ///< Total size of the buffer currently allocated
    uint32_t    m_nData;        ///< Number of bytes currently used in the buffer
    
    /// \brief Ensure that there is sufficient space in the buffer to add an object of the given size
    void EnsureSpace(size_t s);
};

/// \class Serialiser
/// \brief Carry out the serialisation of an object
///
/// This object is intended to write a given \a Serialisable object into the file declared
/// at construction.  The \a payload_id and a size word are automatically added to the
/// output packets.  A packet with the serialiser version information is writtent to each
/// file when it is opened so that readers can check on the expected format of the
/// remainder of the file.

class Serialiser {
public:
    /// \brief Default constructor
    Serialiser(File& f);
    /// \brief Write the payload to file, with header block
    bool Process(uint32_t payload_id, Serialisable const& payload);
    
private:
    File&    m_file;    ///< Reference for the file object to serialise into
    
    /// \brief Payload serialiser without user-level validity checks
    bool rawProcess(uint32_t payload_id, Serialisable const& payload);
};

#endif
