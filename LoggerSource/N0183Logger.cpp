/*!\file N0183Logger.cpp
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

#include "N0183Logger.h"

namespace nmea {
namespace N0183 {

/// Validate the sentence, making sure that it meets the requirements to be a NMEA0183 message.  For
/// this, it has to contain only printable characters, it has to start with a "$", and end with a valid checksum
/// byte (as two hex characters).  If any of these conditions are violated, the sentence is invalid.
///
/// \return True if the sentence if valid, otherwise False.

bool Sentence::Valid(void) const
{
    int checksum = 0;
    
    int i = 1;
    while (i < m_insertPoint && m_sentence[i] != '*') {
        if (!std::isprint(m_sentence[i]))
            // Not printable, so something's wrong
            return false;
        checksum ^= m_sentence[i++];
    }
    if (i == m_insertPoint) {
        // Ran out of string, so there's no checksum to compare against!
        return false;
    }
    char buffer[4];
    sprintf(buffer, "*%02X", checksum);
    buffer[3] = '\0';
    if (strncmp(m_sentence + i, buffer, 4) == 0)
        return true;
    else
        return false;
}

/// Extract from the sentence the recognition token from the start of the NMEA sentence, which includes both
/// the talker identifier, and the sentence name string for a total of five characters.
///
/// \return string of the recognition characters.

std::string Sentence::Token(void) const
{
    std::string s = std::string(m_sentence).substr(1,5);
    return s;
}

/// Start the message assembler in the "searching" state, with a blank sentence and empty FIFO.

MessageAssembler::MessageAssembler(void)
: m_state(STATE_SEARCHING)
{
    m_current = new Sentence();
}

MessageAssembler::~MessageAssembler(void)
{
    delete m_current;
}

/// Take the next character from the input stream, and add to the current sentence, keeping track of the
/// message state (e.g., waiting for a "$", looking for an end-character sequence, etc.  If the sentence comes
/// to an end with the current character (meaning that it has to be a newline (\0x0A), then the sentence is
/// moved into the FIFO, and a new sentence is started.
///
/// \param in   Character being added from the input stream

void MessageAssembler::AddCharacter(const char in)
{
    switch(m_state) {
        case STATE_SEARCHING:
            if (in == '$') {
                // This is the start of a string, so get the timestamp, and stash
                auto now = millis();
                m_current->Reset();
                m_current->Timestamp(now);
                m_current->AddCharacter(in);
                m_state = STATE_CAPTURING;
                if (m_debugAssembly) {
                    Serial.println(String("debug: sentence started with timestamp ") +
                                   m_current->Timestamp() + " on channel " + m_channel +
                                   "; changing to CAPTURING.");
                }
            } else {
                if (m_debugAssembly) {
                    Serial.print("error: non-start character ");
                    if (std::isprint(in)) {
                        Serial.print(String("'") + in + "'");
                    } else {
                        Serial.print("0x");
                        Serial.print(in, HEX);
                    }
                    Serial.println(" while searching for NMEA string.");
                }
            }
            break;
        case STATE_CAPTURING:
            switch(in) {
                case '\n':
                    // The end of the current sentence, so we convert the current sentence
                    // onto the FIFO.  Note that we don't store the carriage return.
                    m_fifo.push(m_current);
                    m_current = new Sentence();
                    if (m_debugAssembly) {
                        Serial.println(String("debug: LF on channel " + m_channel +
                                              " to complete sentence; moved to FIFO");
                    }
                    m_state = STATE_SEARCHING;
                    if (m_debugAssembly) {
                        Serial.println(String("debug: changing to SEARCHING on channel ") +
                                       m_channel + ".");
                    }
                    break;
                case '\r':
                    // The NMEA sentence should complete with CR+LF, but we want to ignore the CR so
                    // that sentence termination is recognised by the LF, but we don't have to deal with
                    // the extra CR in the sentence!
                    break;
                case '$':
                    // The start of a new sentence, which is odd ...
                    auto now = millis();
                    m_current->Reset();
                    m_current->Timestamp(now);
                    m_current->AddCharacter(in);
                    Serial.println("warning: sentence restarted before end of previous one?!");
                    if (m_debugAssembly) {
                        Serial.println("debug: new sentence started with timestamp " +
                                       m_current->Timestamp() + " as reset on channel " +
                                       m_channel + ".");
                    }
                    break;
                default:
                    // The next character of the current sentence
                    if (!m_current->AddCharacter(in)) {
                        // Ran out of buffer space, so return to searching.  Note that we don't need to
                        // reset the current buffer, because the next sentence starting will do so.
                        m_state = STATE_SEARCHING;
                        Serial.println("warning: over-long sentence detected, and ignored.");
                        if (m_debugAssembly) {
                            Serial.println(String("debug: reset state to SEARCHING on channel ") +
                                           m_channel + " after over-long sentence.");
                        }
                    }
                    break;
            }
            break;
        default:
            Serial.println("error: unknown state in message assembly! Resetting to SEARCHING.");
            m_state = STATE_SEARCHING;
            break;
    }

}

/// Initialise a logger structure that can be used to handle all logging capabilities for NMEA0183 data on
/// hardware channels Serial1 and Serial2.  This accumulates data from the serial channels into NMEA0183 sentences,
/// and then logs them as they are terminated.  Invalid sentences are removed from consideration before being
/// logged, and filtering is applied to the data if required in order to minimise the data that is logged.
///
/// \param output   Reference for the output SD file logger to use

Logger::Logger(Logfile::Logger& output)
: m_outputLog(output)
{
    m_channel1 = new MessageAssembler(m_fifo);
    m_channel2 = new MessageAssembler(m_fifo);
}
                                       
Logger::~Logger(void)
{
    delete m_channel1;
    delete m_channel2;
}

/// Handle as many characters as are waiting on the serial channels, using the assemblers to build them
/// into messages, and then checking and serialising when they indicate that a sentence has been completed.
/// This essentially gives a stateless interface to NMEA0183 message handling, since all of the state is
/// encapsulated here.

void Logger::ProcessMessages(void)
{
    while (Serial1.available()) {
        m_channel1.AddCharacter(Serial1.read());
    }
    while (Serial2.available()) {
        m_channel2.AddCharacter(Serial2.read());
    }
    while (~m_fifo.empty()) {
        Sentence *msg = m_fifo.front();
        m_fifo.pop();
        Serialisable s;
        s += (uint64_t)(msg->Timestamp());
        s += msg->Contents();
        m_outputLog.Record(Logfile::MessageType::NMEA0183, s);
        delete msg;
    }
}

}
}
