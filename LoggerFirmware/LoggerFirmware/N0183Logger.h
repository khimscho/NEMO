/*!\file N0183Logger.h
 * \brief Main code for a NMEA0183 message handler with time, position, depth logging and timestamping
 *
 * This code is designed to form a simple data logger for NMEA0183, which keeps track of time and stores
 * position, depth, and some environmental information into the SD card attached to the board.  The goal
 * here is to store sufficient information to make this a useful crowd-sourced bathymetry logging engine,
 * but at the cheapest possible price.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#ifndef __N0183_LOGGER_H__
#define __N1083_LOGGER_H__

#include "LogManager.h"

namespace nmea {
namespace N0183 {

/// \class Sentence
/// \brief Assemble a NMEA0183 sentence from character input on serial line
///
/// A standard NMEA0183 sentence begins with "$" and ends with <CR><LF>, including a checksum
/// at the end.  This structure allows this information to be accumulated as the characters arrive on the
/// input serial stream (by definition RS-422), and buffers the sentences until the user can read them.  A
/// timestamp is generated for the first "$" in each string, and stored with the string.

class Sentence {
public:
    static const int MAX_SENTENCE_LENGTH = 128; ///< Maximum is probably order 100 characters; this is safe
    
    /// \brief Default constructor, just resetting the buffer insertion point
    Sentence(void)
    {
        Reset();
    }
    
    /// \brief Add a new character into the buffer, with length management
    ///
    /// This adds the specified character into the buffer, making sure that there is space
    /// for it beforehand.
    ///
    /// \param a    Character to add
    /// \return True if the character was added, otherwise False
    
    bool AddCharacter(char const a)
    {
        if (m_insertPoint < MAX_SENTENCE_LENGTH-1) {
            m_sentence[m_insertPoint++] = a;
            return true;
        }
        return false;
    }
    
    /// \brief Reset the current sentence, going back to zero length
    ///
    /// This resets the buffer to zero contents, invalidates the timestamp, and sets the insertion
    /// point back to zero, effectively removing the old data without having to reconstruct.
    void Reset(void)
    {
        bzero(m_sentence, MAX_SENTENCE_LENGTH);
        m_timestamp = -1.0;
        m_insertPoint = 0;
    }
    
    /// \brief Provide a pointer to the current sentence in the buffer
    char const *Contents(void) const { return m_sentence; }
    /// \brief Provide a legnth-count for the current contents of the buffer
    int Length(void) const { return m_insertPoint; }
    /// \brief Provide a count for the maximum possible sentence length
    int MaxLength(void) const { return MAX_SENTENCE_LENGTH; }
    
    /// \brief Get the stored timestamp for the sentence
    unsigned long Timestamp(void) const { return m_timestamp; }
    /// \brief Set the stored timestamp from a seconds count
    void Timestamp(unsigned long const t) { m_timestamp = t; }
    
    /// \brief Deteremine whether the sentence is a valid NMEA sentence or not
    bool Valid(void) const;
    
    /// \brief Provide the recognition token from the start of the sentence
    String Token(void) const;
    
private:
    unsigned long   m_timestamp;    ///< Timestamp associated with the "$" that started the sentence
    char            m_sentence[MAX_SENTENCE_LENGTH];    ///< Sentence assembly space
    int             m_insertPoint;  ///< Location for next character to be placed in the buffer
};

/// \class MessageAssembler
/// \brief Implement a state-machine model of accumulating NMEA0183 messages
///
/// This class runs the active part of the NMEA0183 message recognition protocol, keeping track of the
/// state of the channel (searching, in-sentence, etc.) so that it can apply timestamps where required,
/// detect partial messages, early message termination, etc.

class MessageAssembler {
public:
    /// \brief Default constructor
    MessageAssembler(void);
    /// \brief Default destructor
    ~MessageAssembler(void);
    /// \brief Set the channel indicator for reporting
    inline void SetChannel(const int channel) { m_channel = channel; }
    /// \brief Add a new character to the current sentence (potentially completing it)
    void AddCharacter(const char c);
    /// \brief Set debugging state for message assembly
    inline void SetDebugging(const bool state) { m_debugAssembly = state; }
    /// \brief Copy out the next available sentence from the ring buffer
    Sentence const *NextSentence(void);
    
private:
    enum State {
        STATE_SEARCHING,
        STATE_CAPTURING
    };

    static const int RingBufferLength = 10;///< Maximum number of sentences we'll attempt to buffer
    
    State     m_state;                    ///< Current state of the message being assembled
    Sentence  m_current;                  ///< Sentence currently being assembled
    int       m_readPoint;                ///< Ring buffer read position
    int       m_writePoint;               ///< Ring buffer write position
    Sentence  m_buffer[RingBufferLength]; ///< Buffer for sentences being held for use
    int       m_channel;                  ///< Channel indicator for messages
    bool      m_debugAssembly;            ///< Flag for debug message construction
};

/// \class Logger
/// \brief Handle NMEA0183 data on inputs, and transpose full sentences to log file
///
/// This class encapsulates the requirements for a NMEA0183 dual-channel logger, taking any valid NMEA0183 serial
/// strings on hardware serial ports Serial1 and Serial2 and logging them into the output SD card file stream passed
/// on construction.  Timestamps are added appropriately for the first "$" that starts each sentence.
///

class Logger {
public:
    /// \brief Default constructor    
    Logger(logger::Manager *output);
    /// \brief Default destructor
    ~Logger(void);
    
    /// \brief Process any characters that have arrived
    void ProcessMessages(void);

    /// \brief Generate a version string for the logger
    String SoftwareVersion(void) const;
    /// \brief Report the version information for the NMEA0183 logger
    static void SoftwareVersion(uint16_t& major, uint16_t& minor, uint16_t& patch);

    /// \brief Set debugging status
    void SetVerbose(bool verbose);
    
private:
    static const int ChannelCount = 2;            ///< Number of channels that we manage
    bool                m_verbose;                ///< Verbose status for the logger
    logger::Manager    *m_logManager;             ///< Handler for log files on SD card
    MessageAssembler    m_channel[ChannelCount];  ///< Message handler for two channels
};

}
}

#endif
