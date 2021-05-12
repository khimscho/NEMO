/*!\file serialisation.cpp
 * \brief Implement simple serialisation for file storage
 *
 * This implements the interface for serialisation of storage into
 * a WIBL-compatible data file.  This is not intended to be automatic in
 * the tradition of Boost::Serialization, but it's effective, and cheap.
 *
 * Copyright (c) 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
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

#include <memory>
#include <stdint.h>
#include <string>
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

/// Serialise an array of characters to the output buffer (i.e., a C-style, zero
/// terminated, string).
///
/// \param p    Pointer to the array to add

void Serialisable::operator+=(const char *p)
{
    EnsureSpace(strlen(p));
    const char *src = p;
    while (*src != '\0') {
        m_buffer[m_nData++] = *src++;
    }
}

/// Constructor for the serialiser, which writes \a Serialisable objects to file.  The file has
/// to be opened in binary mode in order for this to work effectively.
///
/// \param file Reference for the file on which the data should be serialised.

Serialiser::Serialiser(Version& n2k, Version& n1k, std::string const& logger_name, std::string const& logger_id)
{
    m_version = std::shared_ptr<Serialisable>(new Serialisable(16));
    
    *m_version += (uint16_t)SerialiserVersionMajor;
    *m_version += (uint16_t)SerialiserVersionMinor;
    
    *m_version += n2k.major;
    *m_version += n2k.minor;
    *m_version += n2k.patch;
    
    *m_version += n1k.major;
    *m_version += n1k.minor;
    *m_version += n1k.patch;
    
    m_metadata = std::shared_ptr<Serialisable>(new Serialisable(255));
    
    *m_metadata += static_cast<uint32_t>(logger_name.length());
    *m_metadata += logger_name.c_str();
    *m_metadata += static_cast<uint32_t>(logger_id.length());
    *m_metadata += logger_id.c_str();
}

Serialiser::~Serialiser(void)
{
}

/// User-level method to write the buffer to file.  The payload ID number specified has to be
/// greater than zero (which is reserved for internal use).
///
/// \param payload_id   ID number to write to file in order to identify what's coming next
/// \param payload          Buffer hanlder to be written to file
///
/// \return True if the packet was written to to file, otherwise False

bool Serialiser::Process(PayloadID payload_id, std::shared_ptr<Serialisable> payload)
{
    if (payload_id == Pkt_Version) {
        // Reserved for version packet at the start of the file
        return false;
    }
    
    if (m_version != nullptr) {
        rawProcess(Pkt_Version, m_version);
        m_version.reset();
    }
    if (m_metadata != nullptr) {
        rawProcess(Pkt_Metadata, m_metadata);
        m_metadata.reset();
    }
    
    return rawProcess(payload_id, payload);
}

bool StdSerialiser::rawProcess(PayloadID payload_id, std::shared_ptr<Serialisable> payload)
{
    uint32_t data_size = payload->BufferLength();
    fwrite(&payload_id, sizeof(PayloadID), 1, m_file);
    fwrite(&data_size, sizeof(uint32_t), 1, m_file);
    fwrite(payload->Buffer(), sizeof(uint8_t), data_size, m_file);
    return true;
}
