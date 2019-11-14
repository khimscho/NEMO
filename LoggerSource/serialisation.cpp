/* \file serialisation.cpp
 * \brief Implement simple serialisation for SD file storage
 *
 * This implements the interface for serialisation of storage into
 * an SDLib::File of binary packet data.  This is not intended to be
 * very automatic (on the order of Boost::Serialization), but just
 * enough to get packets into the file
 *
 * 2019-08-25.
 */

#include <stdint.h>
#include <SD.h>
#include "serialisation.h"

Serialisable::Serialisable(uint32_t size_hint)
{
    m_buffer = (uint8_t*)malloc(sizeof(uint8_t)*size_hint);
    m_bufferLength = size_hint;
    m_nData = 0;
}

Serialisable::~Serialisable(void)
{
    free(m_buffer); // Must exist if we get to here
}

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

void Serialisable::operator+=(uint8_t b)
{
    EnsureSpace(sizeof(uint8_t));
    m_buffer[m_nData++] = b;
}

void Serialisable::operator+=(uint16_t h)
{
    EnsureSpace(sizeof(uint16_t));
    uint8_t *data = (uint8_t*)&h;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
}

void Serialisable::operator+=(uint32_t w)
{
    EnsureSpace(sizeof(uint32_t));
    uint8_t *data = (uint8_t*)&w;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
    m_buffer[m_nData++] = data[2];
    m_buffer[m_nData++] = data[3];
}

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

void Serialisable::operator+=(float f)
{
    EnsureSpace(sizeof(float));
    uint8_t *data = (uint8_t*)&f;
    m_buffer[m_nData++] = data[0];
    m_buffer[m_nData++] = data[1];
    m_buffer[m_nData++] = data[2];
    m_buffer[m_nData++] = data[3];
}

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

Serialiser::Serialiser(File& file)
: m_file(file)
{
    Serialisable version(8);
    version += (uint32_t)SerialiserVersionMajor;
    version += (uint32_t)SerialiserVersionMinor;
    rawProcess(0, version);
}

bool Serialiser::rawProcess(uint32_t payload_id, Serialisable& payload)
{
    m_file.write((const uint8_t*)&payload_id, sizeof(uint32_t));
    m_file.write((const uint8_t*)&payload.m_nData, sizeof(uint32_t));
    m_file.write((const uint8_t*)payload.m_buffer, sizeof(uint8_t)*payload.m_nData);
    m_file.flush();
    return true;
}

bool Serialiser::Process(uint32_t payload_id, Serialisable& payload)
{
    if (0 == payload_id) {
        // Reserved for version packet at the start of the file
        return false;
    }
    
    return rawProcess(payload_id, payload);
}
