/*! \file WiFiAdapter.h
 *  \brief Interface for the WiFi adapter on the module, and a factory to abstract the object
 *
 * In order to get to a decent speed for data transfer, we need to use the WiFi interface rather
 * than the BLE.  This provides the interface to abstract the details of this away.
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

#include <functional>
#include <queue>

#include <WiFi.h>
#include <WiFIAP.h>
#include <WebServer.h>

#include "WiFiAdapter.h"
#include "Configuration.h"
#include "MemController.h"
#include "serial_number.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

class ConnectionStateMachine {
public:
    ConnectionStateMachine(void)
    {
        m_lastConnectAttempt = m_lastStatusCheck = millis();
        
        m_connectionRetries = maximumReties();
        m_retryDelay = retryDelay();
        m_connectDelay = connectionDelay();

        m_statusDelay = 500;    // Delay between status checks in milliseconds

        m_currentState = STOPPED;
    }

    void Start(void)
    {
        if (WiFiAdapter::GetWirelessMode() == WiFiAdapter::WirelessMode::ADAPTER_SOFTAP) {
            m_currentState = AP_MODE;
            apSetup();
        } else {
            m_currentState = STATION_CONNECTING;
            if (attemptStationJoin())
                m_currentState = STATION_CONNECTED;
        }
    }

    void StepState(void)
    {
        int now = millis();
        switch (m_currentState) {
            case STOPPED:
                break;
            case AP_MODE:
                // Once in AP mode and established, we stay in the state.
                break;
            case STATION_CONNECTING:
                // We're waiting for the connection to complete, so check status
                // if we've waiting long enough.
                if ((now - m_lastStatusCheck) > m_statusDelay) {
                    if (isConnected()) {
                        m_currentState = STATION_CONNECTED;
                    } else {
                        // Still not connected, so we need to determine if
                        // we need to terminate this try.
                        if ((now - m_lastConnectAttempt) > m_connectDelay) {
                            m_currentState = STATION_RETRY;
                        }
                    }
                }
                break;
            case STATION_RETRY:
                // We're attempting a retry for a station connection, if we've
                // waited long enough
                if ((now - m_lastConnectAttempt) > m_retryDelay) {
                    if (m_connectionRetries > 0) {
                        --m_connectionRetries;
                        if (attemptStationJoin())
                            m_currentState = STATION_CONNECTED;
                        else
                            m_currentState = STATION_CONNECTING;
                    } else {
                        m_currentState = MOVE_TO_SAFE_MODE;
                    }
                }
                break;
            case MOVE_TO_SAFE_MODE:
                // We're out of retries for a station connection, so we have to assume
                // that the network isn't there, or there's a problem with the password
                // etc. -- so we revert to AP mode.
                WiFiAdapter::SetWirelessMode(WiFiAdapter::WirelessMode::ADAPTER_SOFTAP);
                logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_WS_STATUS_S, "AP-Enabled,Station-Join-Failed");
                ESP.restart();
                break;
            case STATION_CONNECTED:
                // The system (finally?) connected, so we update status, and then go into
                // connection checking mode.
                logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_WS_STATUS_S, "Station-Enabled,Connected");
                m_currentState = CONNECTION_CHECK;
                break;
            case CONNECTION_CHECK:
                // Once connected, we need to periodically check that we're still connected,
                // going back into connection mode if not.
                if ((now - m_lastStatusCheck) > m_retryDelay) {
                    if (!isConnected()) {
                        m_currentState = STATION_RETRY;
                    }
                }
                break;
        }
    }
private:
    enum State {
        STOPPED = 0,
        AP_MODE,
        STATION_CONNECTING,
        STATION_CONNECTED,
        STATION_RETRY,
        MOVE_TO_SAFE_MODE,
        CONNECTION_CHECK
    };
    State   m_currentState;         // Current state of the SM
    int     m_lastConnectAttempt;   // Time (ms) for last connection attempt
    int     m_lastStatusCheck;      // Time (ms) for last connection status attempt
    int     m_connectionRetries;    // Count of remaining connection attempts
    int     m_retryDelay;           // Delay (ms) before attempt to retry to connect
    int     m_statusDelay;          // Delay (ms) before checking connection status again
    int     m_connectDelay;         // Delay (ms) before assuming a connect attempt failed

    void apSetup(void)
    {
        String ssid, password;
        logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_AP_SSID_S, ssid);
        logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_AP_PASSWD_S, password);
        WiFi.softAP(ssid.c_str(), password.c_str());
        IPAddress server_address = WiFi.softAPIP();
        logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_WIFIIP_S, server_address.toString());
    }

    bool attemptStationJoin(void)
    {
        String ssid, password;
        logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_STATION_SSID_S, ssid);
        logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_STATION_PASSWD_S, password);
        Serial.print(String("Connecting to ") + ssid + ": ");
        wl_status_t status = WiFi.begin(ssid.c_str(), password.c_str());
        m_lastConnectAttempt = millis();
        Serial.print(String("(status: ") + static_cast<int>(status) + ")");
        return status == WL_CONNECTED;
    }

    void completeStationJoint(void)
    {
        IPAddress server_address = WiFi.softAPIP();
        logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_WIFIIP_S, server_address.toString());
    }

    bool isConnected(void)
    {
        wl_status_t status = WiFi.status();
        m_lastStatusCheck = millis();
        return status == WL_CONNECTED;
    }

    int maximumReties(void)
    {
        String val;
        logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_STATION_RETRIES_S, val);
        return val.toInt();
    }

    int retryDelay(void)
    {
        String val;
        logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_STATION_DELAY_S, val);
        return val.toInt() * 1000;
    }

    int connectionDelay(void)
    {
        String val;
        logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_STATION_TIMEOUT_S, val);
        return val.toInt() * 1000;
    }
};

/// \class ESP32WiFiAdapter
/// \brief Implementation of the WiFiAdapter base class for the ESP32 module

class ESP32WiFiAdapter : public WiFiAdapter {
public:
    /// Default constructor for the ESP32 adapter.  This brings up the parameter store to use
    /// for WiFi parameters, but takes no other action until the user explicitly starts the AccessPoint.
    ESP32WiFiAdapter(void)
    : m_storage(nullptr), m_server(nullptr)
    {
        if ((m_storage = mem::MemControllerFactory::Create()) == nullptr) {
            return;
        }
    }
    /// Default destructor for the ESP32 adapter.  This stops the Access Point, if it isn't already down,
    /// and then deletes the ParamStore manipulation object.
    virtual ~ESP32WiFiAdapter(void)
    {
        stop();
        delete m_storage; // Note that we're not stopping the interface, since it may still be required elsewhere
    }
    
private:
    mem::MemController  *m_storage; ///< Pointer to the storage object to use
    WebServer           *m_server;  ///< Pointer to the server object, if started.
    std::queue<String>  m_commands; ///< Queue to handle commands sent by the user
    String              m_messages; ///< Accumulating message content to be send to the client
    ConnectionStateMachine m_state; ///< Manager for connection state

    /// @brief Handle HTTP requests to the /command endpoint
    ///
    /// HTTP POST endpoint handler for commands being sent to the logger through the web-server
    /// interface.  This simple captures any "command" arguments and queues them for the system
    /// to execute later.
    ///
    /// @return N/A
    void handleCommand(void)
    {
        for (uint32_t i = 0; i < m_server->args(); ++i) {
            if (m_server->argName(i) == "command") {
                m_commands.push(m_server->arg(i));
            }
        }
    }

    /// @brief Provide a simple endpoint for HTTP GET to indicate presence
    ///
    /// It's not always possible to tell whether you have a web-server available on a known IP
    /// address.  This endpoint handle (typically for GET but it should work for anything) returns
    /// a simple text message with the serial number of the logger board and 200OK so that you
    /// know that the server is on and running.
    ///
    /// @return N/A
    void heartbeat(void)
    {
        m_server->send(200, "text/plain", GetSerialNumberString());
    }

    /// Bring up the WiFi adapter, which in this case includes bring up the soft access point.  This
    /// uses the ParamStore to get the information required for the soft-AP, and then interrogates the
    /// server to find out which IP address was allocated.  This is very likely to be the same address
    /// each time (it's usually set by the soft-AP by default), but could change.  The current value is
    /// stored in the NVRAM, but should not be relied on until the server is started.
    ///
    /// \return True if the adapter started as expected; otherwise false.
    
    bool start(void)
    {
        if ((m_server = new WebServer()) == nullptr) {
            Serial.println("ERR: failed to start web server.");
            return false;
        } else {
            // Configure the endpoints served by the server
            using namespace std::placeholders;
            m_server->on("/heartbeat", HTTPMethod::HTTP_GET, std::bind(&ESP32WiFiAdapter::heartbeat, this));
            m_server->on("/command", HTTPMethod::HTTP_POST, std::bind(&ESP32WiFiAdapter::handleCommand, this));
        }
        m_state.Start();
        m_server->begin();
        return true;
    }
    
    /// Stop the WiFi adapter cleanly, and return to BLE only for access to the system.  This attempts to
    /// disconnect any current client cleanly, to avoid broken pipes.
    
    void stop(void)
    {
        delete m_server;
        m_server = nullptr;
    }
    
    /// Read the first terminated (with [LF]) string from the client, and return.  This will read over multiple
    /// packets, etc., until it gets a [LF] character, and then return everything since the last [LF].  The
    /// [LF] is not added to the string, but depending on client, there may be a [CR].
    ///
    /// \return A String object with the contents of the latest read from the client.
    
    String readBuffer(void)
    {
        if (m_commands.size() == 0) {
            return String("");
        } else {
            String rtn = m_commands.front();
            m_commands.pop();
            return rtn;
        }
    }
    
    /// Transfer a specific log file from the logger to the client in an efficient manner that doesn't include
    /// converting from binary to String, etc.  The filename must exist, and there must be a connected
    /// client ready to catch the output, otherwise the call will fail.  The transfer is pretty simple, relying on
    /// efficient implementation of buffer in the libraries for fast transfer.
    ///
    /// \param filename Name of the file to transfer to the client
    /// \return True if the transfer worked, otherwise false.
    
    bool sendLogFile(String const& filename, uint32_t filesize)
    {
        File f = m_storage->Controller().open(filename, FILE_READ);
        if (!f) {
            Serial.println("ERR: failed to open file for transfer.");
            return false;
        } else {
            m_server->streamFile(f, "application/octet-stream");
            f.close();
        }
        return true;
    }

    void accumulateMessage(String const& message)
    {
        m_messages += message;
    }

    bool transmitMessages(char const* data_type)
    {
        m_server->send(200, data_type, m_messages);
        m_messages = "";
        return true;
    }

    void runLoop(void)
    {
        m_state.StepState();
        if (m_server != nullptr)
            m_server->handleClient();
    }
};

#endif

/// Pass-through implementation to sub-class code to start the WiFi adapter.
///
/// \return True if the adapter started, or false.
bool WiFiAdapter::Startup(void) { return start(); }

/// Pass-through implementation to the sub-class code to stop the WiFI adapter.
void WiFiAdapter::Shutdown(void) { stop(); }

/// Pass-through implementation to the sub-class code to retrieve the first available string from the client.
///
/// \return String with the latest LF-terminated data from the client.
String WiFiAdapter::ReceivedString(void) { return readBuffer(); }

/// Pass-through implementation to the sub-class code to transfer a log file to the client.
///
/// \param filename Name of the log file to be transferred.
/// \return True if the file was transferred, otherwise false.
bool WiFiAdapter::TransferFile(String const& filename, uint32_t filesize)
    { return sendLogFile(filename, filesize); }

/// Pass-through implementation to the sub-class code to accumulate messages
///
/// \param message  String for the message to send
/// \return N/A
void WiFiAdapter::AddMessage(String const& message) { accumulateMessage(message); }

/// Pass-through implementation to the sub-class code to transmit all available messages.
/// This should also complete the transaction from the client's request.
///
/// \return True if the messages were transmitted successfully
bool WiFiAdapter::TransmitMessages(char const* data_type) { return transmitMessages(data_type); }

/// Pass-through implementation to the sub-class code to set the wireless adapter mode.
///
/// \param mode WirelessMode for the adapter on startup
void WiFiAdapter::SetWirelessMode(WirelessMode mode)
{
    String value;
    if (mode == WirelessMode::ADAPTER_STATION) {
        value = "Station";
    } else if (mode == WirelessMode::ADAPTER_SOFTAP) {
        value = "AP";
    } else {
        Serial.println("ERR: unknown wireless adapater mode.");
        return;
    }
    if (!logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_WIFIMODE_S, value)) {
        Serial.println("ERR: failed to set WiFi adapater mode on module.");
    }
}

/// Pass-through implementation to the sub-class code to get the wireless adapter mode.
///
/// \return WirelessMode enum value, using AP as the default
WiFiAdapter::WirelessMode WiFiAdapter::GetWirelessMode(void)
{
    String          value;
    WirelessMode    rc;
    
    if (!logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_WIFIMODE_S, value)) {
        Serial.println("ERR: failed to get WiFi adapter mode on module.");
        value = "UNKNOWN";
    }
    if (value == "Station")
        rc = WirelessMode::ADAPTER_STATION;
    else
        rc = WirelessMode::ADAPTER_SOFTAP;
    return rc;
}

void WiFiAdapter::RunLoop(void) { runLoop(); }

/// Create an implementation of the WiFiAdapter interface that's appropriate for the hardware in use
/// in the current logger module.
///
/// \return Pointer to the adapter, cast to the base WiFiAdapter.
WiFiAdapter *WiFiAdapterFactory::Create(void)
{
    WiFiAdapter *obj;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    obj = new ESP32WiFiAdapter();
#else
#error "No WiFi Adapter supported for hardware."
#endif
    return obj;
}
