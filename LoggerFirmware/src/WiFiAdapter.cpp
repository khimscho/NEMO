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

    void handleCommand(void)
    {
        Serial.printf("DBG: received HTTP request %d for %d arguments.\n", (int)m_server->method(), m_server->args());
        for (uint32_t i = 0; i < m_server->args(); ++i) {
            if (m_server->argName(i) == "command") {
                m_commands.push(m_server->arg(i));
            }
        }
    }

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
        if (get_wireless_mode() == WirelessMode::ADAPTER_SOFTAP) {
            String ssid, password;
            logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_AP_SSID_S, ssid);
            logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_AP_PASSWD_S, password);
            WiFi.softAP(ssid.c_str(), password.c_str());
            IPAddress server_address = WiFi.softAPIP();
            logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_WIFIIP_S, server_address.toString());
        } else {
            String ssid, password;
            logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_CLIENT_SSID_S, ssid);
            logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_CLIENT_PASSWD_S, password);
            Serial.print(String("Connecting to ") + ssid + ": ");
            wl_status_t status = WiFi.begin(ssid.c_str(), password.c_str());
            Serial.print(String("(status: ") + static_cast<int>(status) + ")");
            uint32_t wait_loops = 0;
            uint32_t retry_maximum;
            String retries;
            logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_WS_REBOOTS_S, retries);
            retry_maximum = retries.toInt();
            // TODO: Make this loop something that can be done in the background while we log
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
                ++wait_loops;
                if (wait_loops > retry_maximum) {
                    Serial.printf("\nINFO: Failed to connect on WiFi.\n");
                    return false;
                }
            }
            Serial.println("");
            IPAddress server_address = WiFi.localIP();
            logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_WIFIIP_S, server_address.toString());
        }
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

    /// Sets the mode in which to bring up the interface.  Most WiFi embedded solutions can come up either
    /// as a client on some other WiFi network, or as an access point ("Soft AP"), making its own network.  The
    /// default condition is to come up as an AP, but for debug, and in some other applications, it might make
    /// more sense to come up as a client.
    ///
    /// \param  mode    WirelessMode enum to use for the next setup
    
    void set_wireless_mode(WirelessMode mode)
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
    
    /// Gets the current configured mode for the WiFi adapter.  By default, the system should come up as an
    /// access point ("SoftAP") that makes its own network, but can come up as a client on another network
    /// if required.
    ///
    /// \return WirelessMode enum type for the current configuration; returns ADAPTER_SOFTAP by default
    
    WirelessMode get_wireless_mode(void)
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

    void runLoop(void)
    {
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
void WiFiAdapter::SetWirelessMode(WirelessMode mode) { return set_wireless_mode(mode); }

/// Pass-through implementation to the sub-class code to get the wireless adapter mode.
///
/// \return WirelessMode enum value, using AP as the default
WiFiAdapter::WirelessMode WiFiAdapter::GetWirelessMode(void) { return get_wireless_mode(); }

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
