/*!\file serialisation.cpp
 * \brief Implement simple serialisation for SD file storage
 *
 * This implements the interface for serialisation of storage into
 * an SDLib::File of binary packet data.  This is not intended to be
 * very automatic (on the order of Boost::Serialization), but just
 * enough to get packets into the file
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping
 * and NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#include <stdint.h>
#include <SD.h>
#include "serialisation.h"

/// Constructor for a serialisable buffer of data.  The buffer is allocated to the \a size_hint but
/// can grow as new data is added if required.  Expanding a buffer is expensive, so it's wise to
/// set the size of the buffer appropriately at construction, if the nominal size is know.
///
/// \param size_hint    Starting buffer size

Serialisable::Serialisable(uint32_t size_hint)
{
    m_buffer = (uint8_t*)malloc(sizeof(uint8_t)*size_hint);
    m_bufferLength = size_hint;
    m_nData = 0;
}

/// Destructor for the serialisation buffer.  This simply frees the buffer allocated to avoid leaks.

Serialisable::~Serialisable(void)
{
    free(m_buffer); // Must exist if we get to here
}

/// Make sure that there is sufficient space in the buffer to include the data that's about to
/// be added to the buffer, and re-allocate if not.  Re-allocation is expensive, so you want to
/// avoid this if possible by setting the size hint on the constructor.
///
/// \param s    Size in bytes of the data that's about to be added.

void Serialisable::EnsureSpace(size_t s)
{
    if ((m_bufferLength - m_nData) < s) {
        uint8_t *new_buffer = (uint8_t *)malloc(sizeof(uint8_t)*(2*m_bufferLength));
        memcpy(new_buffer, m_buffer, sizeof(uint8_t)*m_nData);
        free(m_buffer);
        m_buffer = new_buffer;
        m_bufferLength = 2*m_bufferLength;
    }
}

/// Serialise a byte into the buffer.
///
/// \param b    Unsigned byte to be added to the buffer.

void Serialisable::operator+=(uint8_t b)
{
    EnsureSpace(sizeof(uint8_t));
    m_buffer[m_nData++] = b;
}

/// Serialise a half-word to the buffer.
///
/// \param h    Half-word (16-bit unsigned) to be added to the buffer.

void Serialisable::operator+=(uint16_t h)
{
    EnsureSpace(sizeof(uint16_t));
    uint8_t *data = (uint8_t*)&h;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
}

/// Serialise a word (32-bit) to the buffer.
///
/// \param w    Word (32-bit unsigned) to be added to the buffer

void Serialisable::operator+=(uint32_t w)
{
    EnsureSpace(sizeof(uint32_t));
    uint8_t *data = (uint8_t*)&w;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
    m_buffer[m_nData++] = data[2];
    m_buffer[m_nData++] = data[3];
}

/// Serialise a double-word (64-bit) to the buffer
///
/// \param ul   Double-word (64-bit unsigned) to be added to the buffer

void Serialisable::operator+=(uint64_t ul)
{
    EnsureSpace(sizeof(uint64_t));
    uint8_t *data = (uint8_t*)&ul;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
    m_buffer[m_nData++] = data[2];
    m_buffer[m_nData++] = data[3];
    m_buffer[m_nData++] = data[4];
    m_buffer[m_nData++] = data[5];
    m_buffer[m_nData++] = data[6];
    m_buffer[m_nData++] = data[7];
}

/// Serialise a single-precision floating point (32-bit) to the buffer
///
/// \param f    Single-precision float (32-bit) to be added to the buffer.

void Serialisable::operator+=(float f)
{
    EnsureSpace(sizeof(float));
    uint8_t *data = (uint8_t*)&f;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
    m_buffer[m_nData++] = data[2];
    m_buffer[m_nData++] = data[3];
}

/// Serialise a double-precision floating point (64-bit) to the buffer
///
/// \param d    Double-precision float (64-bit) to be added to the buffer

void Serialisable::operator+=(double d)
{
    EnsureSpace(sizeof(double));
    uint8_t *data = (uint8_t*)&d;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
    m_buffer[m_nData++] = data[2];
    m_buffer[m_nData++] = data[3];
    m_buffer[m_nData++] = data[4];
    m_buffer[m_nData++] = data[5];
    m_buffer[m_nData++] = data[6];
    m_buffer[m_nData++] = data[7];
}

/// Constructor for the serialiser, which writes \a Serialisable objects to file.  The file has
/// to be opened in binary mode in order for this to work effectively.
///
/// \param file Reference for the file on which the data should be serialised.

Serialiser::Serialiser(File& file)
: m_file(file)
{
    Serialisable version(8);
    version += (uint32_t)SerialiserVersionMajor;
    version += (uint32_t)SerialiserVersionMinor;
    rawProcess(0, version);
}

/// Private method to actually write the buffer to file.  This avoids cross-checks on the payload ID
/// specified so that we can write the ID 0 packet for the serialiser version.
///
/// \param payload_id   ID number to write to file in order to identify what's coming next
/// \param payload          Buffer handler to be written to file

bool Serialiser::rawProcess(uint32_t payload_id, Serialisable& payload)
{
    m_file.write((const uint8_t*)&payload_id, sizeof(uint32_t));
    m_file.write((const uint8_t*)&payload.m_nData, sizeof(uint32_t));
    m_file.write((const uint8_t*)payload.m_buffer, sizeof(uint8_t)*payload.m_nData);
    m_file.flush();
    return true;
}

/// User-level method to write the buffer to file.  The payload ID number specified has to be
/// greater than zero (which is reserved for internal use).
///
/// \param payload_id   ID number to write to file in order to identify what's coming next
/// \param payload          Buffer hanlder to be written to file

bool Serialiser::Process(uint32_t payload_id, Serialisable& payload)
{
    if (0 == payload_id) {
        // Reserved for version packet at the start of the file
        return false;
    }
    
    return rawProcess(payload_id, payload);
}
