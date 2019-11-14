/* \file serialisation.h
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
 * 2019-08-25.
 */

#ifndef __SERIALISATION_H__
#define __SERIALISATION_H__

#include <stdint.h>
#include <SD.h>

const int SerialiserVersionMajor = 1;
const int SerialiserVersionMinor = 0;

/// \class Serialisable
/// \brief Provide encapsulation for data to be written to store

class Serialisable {
public:
    Serialisable(uint32_t size_hint = 255);
    ~Serialisable(void);
    
    void operator+=(uint8_t b);
    void operator+=(uint16_t w);
    void operator+=(uint32_t dw);
    void operator+=(uint64_t ul);
    void operator+=(float f);
    void operator+=(double d);
    
private:
    friend class Serialiser;
    uint8_t     *m_buffer;
    uint32_t    m_bufferLength;
    uint32_t    m_nData;
    
    void EnsureSpace(size_t s);
};

/// \class Serialiser
/// \brief Carry out the serialisation of an object

class Serialiser {
public:
    Serialiser(File& f);
    bool Process(uint32_t payload_id, Serialisable& payload);
    
private:
    File&    m_file;
    bool rawProcess(uint32_t payload_id, Serialisable& payload);
};

#endif
