/*!\file N0183Logger.cpp
 * \brief Main code for a NMEA0183 message handler with time, position, depth logging and timestamping
 *
 * This code is designed to form a simple data logger for NMEA0183, which keeps track of time and stores
 * position, depth, and some environmental information into the SD card attached to the board.  The goal
 * here is to store sufficient information to make this a useful crowd-sourced bathymetry logging engine,
 * but at the cheapest possible price.
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

#include "Arduino.h"
#include <string>
#include <cstring>
#include "N0183Logger.h"
#include "Configuration.h"

namespace nmea {
namespace N0183 {
  
const int SoftwareVersionMajor = 1; ///< Software major version for the logger
const int SoftwareVersionMinor = 0; ///< Software minor version for the logger
const int SoftwareVersionPatch = 1; ///< Software patch version for the logger

/// Validate the sentence, making sure that it meets the requirements to be a NMEA0183 message.  For
/// this, it has to contain only printable characters, it has to start with a "$", and end with a valid checksum
/// byte (as two hex characters).  If any of these conditions are violated, the sentence is invalid.
///
/// \return True if the sentence if valid, otherwise False.

bool Sentence::Valid(void) const
{
    int checksum = 0;
    
    int i = 1;
    while (i < InsertPoint() && BufferChar(i) != '*') {
        if (!std::isprint(BufferChar(i)))
            // Not printable, so something's wrong
            return false;
        checksum ^= BufferChar(i++);
    }
    if (i == InsertPoint()) {
        // Ran out of string, so there's no checksum to compare against!
        return false;
    }
    char buffer[4];
    sprintf(buffer, "*%02X", checksum);
    buffer[3] = '\0';
    if (strncmp(Contents() + i, buffer, 4) == 0)
        return true;
    else
        return false;
}

/// Extract from the sentence the recognition token from the start of the NMEA sentence, which includes both
/// the talker identifier, and the sentence name string for a total of five characters.
///
/// \return string of the recognition characters.

String Sentence::Token(void) const
{
    String s = String(Contents()).substring(1,6);
    return s;
}

/// Start the message assembler in the "searching" state, with a blank sentence and empty FIFO.

MessageAssembler::MessageAssembler(void)
: m_logManager(nullptr), m_state(STATE_SEARCHING), m_readPoint(0), m_writePoint(0), m_channel(-1),
  m_debugAssembly(false), m_badStartCount(0), m_lastInvertResetTime(millis())
{
}

MessageAssembler::~MessageAssembler(void)
{
}

/// Take the next character from the input stream, and add to the current sentence, keeping track of the
/// message state (e.g., waiting for a "$", looking for an end-character sequence, etc.  If the sentence comes
/// to an end with the current character (meaning that it has to be a newline (\0x0A), then the sentence is
/// moved into the FIFO, and a new sentence is started.
///
/// \param in   Character being added from the input stream

void MessageAssembler::AddCharacter(const char in)
{
    String message;
    
    switch(m_state) {
        case STATE_SEARCHING:
            if (in == '$') {
                // This is the start of a string, so get the timestamp, and stash
                auto now = millis();
                m_current.Reset();
                m_current.Timestamp(now);
                m_current.AddCharacter(in);
                m_state = STATE_CAPTURING;
                if (m_debugAssembly) {
                    Serial.println(String("debug: sentence started with timestamp ") +
                                   m_current.Timestamp() + " on channel " + m_channel +
                                   "; changing to CAPTURING.");
                }
            } else {
                if (m_debugAssembly || m_logManager != nullptr) {
                    message = "ERR: non-start character ";
                    if (std::isprint(in)) {
                        message += String("'") + in + "'";
                    } else {
                        message += "0x" + String(in, HEX);
                    }
                    message += " while searching for NMEA string (channel " + String(m_channel) + ").";
                    if (m_debugAssembly) {
                        Serial.println(message);
                    }
                    if (m_logManager != nullptr) {
                        m_logManager->Console().println(message);
                    }
                }

                if ((in & 0x80) != 0) {
                    // Top bit should never be set in serial ASCII, so that's either a noise
                    // hit, or we might have a polarity problem.  We count these for now,
                    // so that we can try to invert later.
                    m_badStartCount++;
                }
                auto elapsed_time = millis() - m_lastInvertResetTime;
                if (elapsed_time == 0) ++elapsed_time;
                if ((m_badStartCount * 1000 / elapsed_time) > 10) {
                    // If we're seeing more than 10 bad starts/second, it's likely that we've
                    // got an inversion of the inputs, so we attempt to fix that.
                    m_lastInvertResetTime = millis();
                    m_badStartCount = 0;
                    if (m_channel == 1)
                        Serial1.setRxInvert(true);
                    else if (m_channel == 2)
                        Serial2.setRxInvert(true);
                    message = "INFO: setting rx input inversion on channel "
                                + String(m_channel) + " due to bad start characters.";
                    Serial.println(message);
                    if (m_logManager != nullptr) {
                        m_logManager->Console().println(message);
                    }
                }
            }
            break;
        case STATE_CAPTURING:
            unsigned int now;
            switch(in) {
                case '\n':
                    // The end of the current sentence, so we convert the current sentence
                    // onto the ring buffer.  Note that we don't store the carriage return.
                    m_buffer[m_writePoint] = m_current;
                    m_writePoint = (m_writePoint + 1) % RingBufferLength;
                    if (m_debugAssembly) {
                        Serial.println(String("debug: LF on channel ") + m_channel +
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
                    now = millis();
                    m_current.Reset();
                    m_current.Timestamp(now);
                    m_current.AddCharacter(in);
                    message = "WARN: sentence restarted before end of previous one?! (channel " + String(m_channel) + ").";
                    Serial.println(message);
                    if (m_logManager != nullptr) {
                        m_logManager->Console().println(message);
                    }
                    if (m_debugAssembly) {
                        Serial.println(String("debug: new sentence started with timestamp ") +
                                       m_current.Timestamp() + " as reset on channel " +
                                       m_channel + ".");
                    }
                    break;
                default:
                    // The next character of the current sentence
                    if (!m_current.AddCharacter(in)) {
                        // Ran out of buffer space, so return to searching.  Note that we don't need to
                        // reset the current buffer, because the next sentence starting will do so.
                        m_state = STATE_SEARCHING;
                        message = "WARN: over-long sentence detected, and ignored (channel " + String(m_channel) + ").";
                        Serial.println(message);
                        if (m_logManager != nullptr) {
                            m_logManager->Console().println(message);
                        }
                        if (m_debugAssembly) {
                            Serial.println(String("debug: reset state to SEARCHING on channel ") +
                                           m_channel + " after over-long sentence.");
                        }
                    }
                    break;
            }
            break;
        default:
            Serial.println("ERR: unknown state in message assembly! Resetting to SEARCHING.");
            if (m_logManager != nullptr) {
                m_logManager->Console().println("ERR: unknown state in message assembly! Resetting to SEARCHING.");
            }
            m_state = STATE_SEARCHING;
            break;
    }
}

/// Pull the next sentence out of the ring buffer (as a pointer to the location of the message),
/// and update the buffer.  A new sentence only exists if there's data; if there's nothing left,
/// a null pointer is returned.
///
/// \return Pointer to the next sentence, or nullptr

Sentence const *MessageAssembler::NextSentence(void)
{
    if (m_readPoint == m_writePoint) return nullptr;
    
    Sentence const *rtn = m_buffer + m_readPoint;
    m_readPoint = (m_readPoint + 1) % RingBufferLength;

    return rtn;
}
                                       
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
#if defined(PROTOTYPE_LOGGER)
const int rx1_pin = 13;
const int tx1_pin = 32;
const int rx2_pin = 14;
const int tx2_pin = 33;
#elif defined(BUILD_NEMO30)
const int rx1_pin = 27; ///< UART port 1 receive pin number (default for this system, not standard)
const int tx1_pin = 18; ///< UART port 1 transmit pin number (default for this system, not standard)
const int rx2_pin = 33; ///< UART port 2 receive pin number (default for this system, not standard)
const int tx2_pin = 19; ///< UART port 2 transmit pin number (default for this system, not standard)
#else
const int rx1_pin = 34; ///< UART port 1 receive pin
const int tx1_pin = 18; ///< UART port 1 transmit pin
const int rx2_pin = 35; ///< UART port 2 receive pin
const int tx2_pin = 19; ///< UART port 2 transmit pin
#endif
#elif defined(__SAM3X8E__)
// Note that these are the defaults, since there doesn't appear to be a way to adjust on Arduino Due
const int rx1_pin = 19; ///< UART port 1 receive pin
const int tx1_pin = 18; ///< UART port 1 transmit pin
const int rx2_pin = 17; ///< UART port 2 receive pin
const int tx2_pin = 16; ///< UART port 2 transmit pin
#else
#error "No configuration recognised for serial inputs"
#endif

/// Initialise a logger structure that can be used to handle all logging capabilities for NMEA0183 data on
/// hardware channels Serial1 and Serial2.  This accumulates data from the serial channels into NMEA0183 sentences,
/// and then logs them as they are terminated.  Invalid sentences are removed from consideration before being
/// logged, and filtering is applied to the data if required in order to minimise the data that is logged.
///
/// \param output   Reference for the output SD file logger to use

Logger::Logger(logger::Manager *output)
: m_verbose(false), m_logManager(output)
{
    m_channel[0].SetChannel(1);
    m_channel[0].SetLogManager(output);
    m_channel[1].SetChannel(2);
    m_channel[1].SetLogManager(output);
    
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    int used_tx1_pin = tx1_pin, used_tx2_pin = tx2_pin;
#if defined(BUILD_NEMO30)
    // We have limits on the number of pins that can be used, so the TX pins (which we don't need)
    // are routed to the SPI pins by default on the assumption that the eMMC module will be used
    // instead for storage, and the SPI pins can be safely reused.  Of course, if that isn't the
    // case, we need to switch ...
    bool sdio_in_use = false;
    if (!logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_SDMMC_B, sdio_in_use)) {
        sdio_in_use = false;
    }
    if (!sdio_in_use) {
        // Reassign to eMMC data lines not otherwise being used.
        used_tx1_pin = 14;
        used_tx2_pin = 15;
    }
#endif
    Serial1.begin(4800, SERIAL_8N1, rx1_pin, used_tx1_pin);
    Serial2.begin(4800, SERIAL_8N1, rx2_pin, used_tx2_pin);
#elif defined(__SAM3X8E__)
    Serial1.begin(4800);
    Serial2.begin(4800);
#endif
}
                                       
Logger::~Logger(void)
{
}

/// Handle as many characters as are waiting on the serial channels, using the assemblers to build them
/// into messages, and then checking and serialising when they indicate that a sentence has been completed.
/// This essentially gives a stateless interface to NMEA0183 message handling, since all of the state is
/// encapsulated here.

void Logger::ProcessMessages(void)
{
    Sentence const *sentence;
    while (Serial1.available()) {
        m_channel[0].AddCharacter(Serial1.read());
    }
    while (Serial2.available()) {
        m_channel[1].AddCharacter(Serial2.read());
    }
    for (int channel = 0; channel < ChannelCount; ++channel) {
        while ((sentence = m_channel[channel].NextSentence()) != nullptr) {
            if (m_verbose) {
                Serial.println(String("debug: logging ") + sentence->Contents());
            }
            Serialisable s;
            s += (uint32_t)(sentence->Timestamp());
            s += sentence->Contents();
            m_logManager->Record(logger::Manager::PacketIDs::Pkt_NMEAString, s);
        }
    }
}

/// Assemble a logger version string
///
/// \return Printable version of the version information

String Logger::SoftwareVersion(void) const
{
    String rtn;
    rtn = String(SoftwareVersionMajor) + "." + String(SoftwareVersionMinor) +
            "." + String(SoftwareVersionPatch);
    return rtn;
}

/// Report the versioning information for the logger, primarily so that it can be written
/// into the log file to allow the reader to adapt to the contents during read.
///
/// \param major    Major software version
/// \param minor    Minor software version
/// \param patch    Patch level/build level

void Logger::SoftwareVersion(uint16_t& major, uint16_t& minor, uint16_t& patch)
{
    major = SoftwareVersionMajor;
    minor = SoftwareVersionMinor;
    patch = SoftwareVersionPatch;
}

/// Set the verbose reporting status of the logger (and the message assemblers)
///
/// \param verbose  Set to true for detailed information, or false for quiet

void Logger::SetVerbose(bool verbose)
{
    m_verbose = verbose;
    for (int ch = 0; ch < ChannelCount; ++ch) {
        m_channel[ch].SetDebugging(verbose);
    }
}

void Logger::SetRxInvert(uint32_t port, bool invert)
{
    if (port < 1 || port > ChannelCount) {
        Serial.println(String("ERR: can't set rx-invert for port ") + port);
    } else {
        if (port == 1)
            Serial1.setRxInvert(invert);
        else if (port == 2)
            Serial2.setRxInvert(invert);
    }
}

}
}
