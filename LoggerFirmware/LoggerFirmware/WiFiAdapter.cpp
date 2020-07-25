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

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFIAP.h>
#include <SD.h>

#include "WiFiAdapter.h"
#include "ParamStore.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

/// \const Port number to use for the server (assumed pre-shared).
const int server_port_number = 25443;

/// \class ESP32WiFiAdapter
/// \brief Implementation of the WiFiAdapter base class for the ESP32 module

class ESP32WiFiAdapter : public WiFiAdapter {
public:
    /// Default constructor for the ESP32 adapter.  This brings up the parameter store to use
    /// for WiFi parameters, but takes no other action until the user explicitly starts the AccessPoint.
    ESP32WiFiAdapter(void)
    : m_paramStore(nullptr), m_server(nullptr)
    {
        if ((m_paramStore = ParamStoreFactory::Create()) == nullptr) {
            return;
        }
    }
    /// Default destructor for the ESP32 adapter.  This stops the Access Point, if it isn't already down,
    /// and then deletes the ParamStore manipulation object.
    virtual ~ESP32WiFiAdapter(void)
    {
        stop();
        delete m_paramStore;
    }
    
private:
    ParamStore  *m_paramStore;  ///< Pointer to the object used to manipulate the key/value parameter store.
    WiFiServer  *m_server;      ///< Pointer to the server object, if started.
    WiFiClient  m_client;       ///< Pointer to the current client connection, if there is one.
    
    /// Bring up the WiFi adapter, which in this case includes bring up the soft access point.  This
    /// uses the ParamStore to get the information required for the soft-AP, and then interrogates the
    /// server to find out which IP address was allocated.  This is very likely to be the same address
    /// each time (it's usually set by the soft-AP by default), but could change.  The current value is
    /// stored in the NVRAM, but should not be relied on until the server is started.
    ///
    /// \return True if the adapter started as expected; otherwise false.
    
    bool start(void)
    {
        if ((m_server = new WiFiServer(server_port_number)) == nullptr) {
            Serial.println("ERR: failed to start WiFi server.");
            return false;
        }
        if (get_wireless_mode() == WirelessMode::ADAPTER_SOFTAP) {
            WiFi.softAP(get_ssid().c_str(), get_password().c_str());
            IPAddress server_address = WiFi.softAPIP();
            set_address(server_address);
        } else {
            Serial.print(String("Connecting to ") + get_ssid() + ": ");
            wl_status_t status = WiFi.begin(get_ssid().c_str(), get_password().c_str());
            Serial.print(String("(status: ") + static_cast<int>(status) + ")");
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
            }
            Serial.println("");
            IPAddress server_address = WiFi.localIP();
            set_address(server_address);
        }
        m_server->begin();
    }
    
    /// Stop the WiFi adapter cleanly, and return to BLE only for access to the system.  This attempts to
    /// disconnect any current client cleanly, to avoid broken pipes.
    
    void stop(void)
    {
        if (m_client) {
            m_client.stop();
        }
        m_server->end();
        delete m_server;
        m_server = nullptr;
    }
    
    /// Check whether there is a client already, or one waiting to be connected, and return an
    /// indication of whether there is some work likely to be required.
    ///
    /// \return True if there is a current client, or false.
    
    bool isConnected(void)
    {
        if (m_server == nullptr)
            return false;
        
        if (!m_client) {
            m_client = m_server->available();
        }
        if (m_client) {
            if (m_client.connected()) {
                return true;
            } else {
                m_client.stop();
                return false;
            }
        }
        return false;
    }
    
    /// Provide an indication of whethere there is data waiting to be read from the client.  This
    /// implicitly indicates that a client is connected.
    ///
    /// \return True if there is a client with data waiting, otherwise false.
    
    bool dataCount(void)
    {
        if (isConnected() && m_client.available() > 0) {
            return true;
        } else {
            return false;
        }
    }
    
    /// Read the first terminated (with [LF]) string from the client, and return.  This will read over multiple
    /// packets, etc., until it gets a [LF] character, and then return everything since the last [LF].  The
    /// [LF] is not added to the string, but depending on client, there may be a [CR].
    ///
    /// \return A String object with the contents of the latest read from the client.
    
    String readBuffer(void)
    {
        String buffer;
        if (!isConnected()) {
            m_client.stop();
            return buffer;
        }
        while (m_client.available() > 0) {
            char c = m_client.read();
            if (c == '\n') {
                return buffer;
            } else {
                buffer += c;
            }
        }
    }
    
    /// Transfer a specific log file from the logger to the client in an efficient manner that doesn't include
    /// converting from binary to String, etc.  The filename must exist, and there must be a connected
    /// client ready to catch the output, otherwise the call will fail.  The transfer is pretty simple, relying on
    /// efficient implementation of buffer in the libraries for fast transfer.
    ///
    /// \param filename Name of the file to transfer to the client
    /// \return True if the transfer worked, otherwise false.
    
    bool sendLogFile(String const& filename)
    {
        if (!isConnected()) return false;
        SDFile f = SD.open(filename, FILE_READ);
        if (!f) {
            Serial.println("ERR: failed to open file for transfer.");
            return false;
        } else {
            while (f.available()) {
                m_client.write(f.read());
            }
            f.close();
        }
        return true;
    }
    
    /// Set the NVRAM version of the SSID for the WiFi adapter.  This provides the mechanism to cache the
    /// SSID that the user wants the soft access point to use when it comes, making it simpler for the client
    /// application to determine which wireless network to connect.  There is no checking on the string being
    /// used, since there are apparently no restrictions on what an SSID can be.  It does have to be a valid
    /// String, however, so it can't have any embedded null bytes.
    ///
    /// \param ssid The SSID string to present to the soft-AP.
    
    void set_ssid(String const& ssid)
    {
        if (!m_paramStore->SetKey("ssid", ssid)) {
            Serial.println("ERR: failed to set SSID string on module.");
        }
    }
    
    /// Return the SSID set by the end-user application for the WiFi adapter.  This could be any set of bytes
    /// in theory, but in practice here has to be a valid string, and therefore is a little more constrained.
    ///
    /// \return String with the SSID, or "UNKNOWN" if it hasn't been set.
    
    String get_ssid(void)
    {
        String value;
        if (!m_paramStore->GetKey("ssid", value)) {
            Serial.println("ERR: failed to get SSID string from module.");
            value = "UNKNOWN";
        }
        return value;
    }
    
    /// Sets the password for the WiFi access point in NVRAM.  In worst-practice, this is currently stored in
    /// plain-text in the NVRAM for the module, and therefore could be retrieved by anyone with physical
    /// access to the module.  That might change in the future, but at least at the level of the proof of concept,
    /// it shouldn't cause too much hassle.
    ///
    /// \param password String to use for the WiFi password (i.e., to joint the network).
    
    void set_password(String const& password)
    {
        if (!m_paramStore->SetKey("password", password)) {
            Serial.println("ERR: failed to set password on module.");
        }
    }
    
    /// Gets the password for the WiFi access point from the NVRAM.  Since this is currently stored in
    /// plain-text, it is returned in the same way.  In practice, this is unlikely to be a major problem for the proof
    /// of concept project, although it might need to change in the future.  The rationale for having this
    /// code, however, is so that the module can tell the end user (over BLE) what the password for the access
    /// point should be, so that the system can log on successfully, and therefore having the password in
    /// plain-text at some point is required.  Since an unknown logger isn't necessarily going to have a recent
    /// password, it would be problematic to find the right password without something like this.
    ///
    /// \return String with the plain-text password to join the WiFi access point network.
    
    String get_password(void)
    {
        String value;
        if (!m_paramStore->GetKey("password", value)) {
            Serial.println("ERR: failed to get password on module.");
            value = "UNKNOWN";
        }
        return value;
    }
    
    /// Sets the IP address key for the WiFi adapter in NVRAM.  This address is only guaranteed while the WiFi server
    /// is instantiated, since in theory it could change each time the server is instantiated.  In practice, that's unlikely
    /// but possible.  Note that this is stored as a simple string rather than attempting to serialise the IP address and
    /// then reconstruct on recovery.  That means a little more work for the client, but nothing there isn't a library for.
    ///
    /// \param address  The IP (v4) address assigned to the server by the soft-AP on boot.
    
    void set_address(IPAddress const& address)
    {
        if (!m_paramStore->SetKey("ipaddress", address.toString())) {
            Serial.println("ERR: failed to set WiFI IP address on module.");
        }
    }
    
    /// Gets the address for the WiFi server from NVRAM.  This value is really only valid when the server is instantiated,
    /// so an arbitrary read of this could give you something from ancient history.  Note that this is stored as a "dotted
    /// notation" IP (v4) address.
    ///
    /// \return String with the IP (v4) address of the sever.
    
    String get_address(void)
    {
        String value;
        if (!m_paramStore->GetKey("ipaddress", value)) {
            Serial.println("ERR: failed to get WiFi IP address on module.");
            value = "UNKNOWN";
        }
        return value;
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
        if (!m_paramStore->SetKey("wifimode", value)) {
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
        
        if (!m_paramStore->GetKey("wifimode", value)) {
            Serial.println("ERR: failed to get WiFi adapter mode on module.");
            value = "UNKNOWN";
        }
        if (value == "Station")
            rc = WirelessMode::ADAPTER_STATION;
        else
            rc = WirelessMode::ADAPTER_SOFTAP;
        return rc;
    }
    
    /// Provide a Stream interface for the client (to read/write data).  Use with caution.
    ///
    /// \return WiFiClient reference up-cast to Stream.
    
    Stream& get_client_stream(void)
    {
        return m_client;
    }
};

#endif

/// Pass-through implementation to sub-class code to start the WiFi adapter.
///
/// \return True if the adapter started, or false.
bool WiFiAdapter::Startup(void) { return start(); }

/// Pass-through implementation to the sub-class code to stop the WiFI adapter.
void WiFiAdapter::Shutdown(void) { stop(); }

/// Pass-through implementation to the sub-class code to check on whether a client is available.
///
/// \return True if there is a client available for connection or already connected.
bool WiFiAdapter::IsConnected(void) { return isConnected(); }

/// Pass-through implementation to the sub-class code to check for data availability from the client.
/// This also implicitly demonstrates that there is a client connected.
///
/// \return True if there is a client, and data available, otherwise false.
bool WiFiAdapter::DataAvailable(void) { return dataCount() > 0; }

/// Pass-through implementation to the sub-class code to retrieve the first available string from the client.
///
/// \return String with the latest LF-terminated data from the client.
String WiFiAdapter::ReceivedString(void) { return readBuffer(); }

/// Pass-through implementation to the sub-class code to transfer a log file to the client.
///
/// \param filename Name of the log file to be transferred.
/// \return True if the file was transferred, otherwise false.
bool WiFiAdapter::TransferFile(String const& filename) { return sendLogFile(filename); }

/// Pass-through implementation to the sub-class code to get the SSID in use for the WiFi adapter.
///
/// \return String containing the SSID for the WiFi adapter.
String WiFiAdapter::GetSSID(void) { return get_ssid(); }

/// Pass-through implementation to the sub-class code to set the SSID for the WiFi adapter.  The
/// string passed is not validated in any way, as there is no real standard for what the SSID should
/// be.  It does, however, have to be a valid String, which means that it can't have any mid-string
/// null characters, etc.
///
/// \param ssid String to use for the SSID.
void WiFiAdapter::SetSSID(String const& ssid) { return set_ssid(ssid); }

/// Pass-through implementation to the sub-class code to get the password to use for the WiFi adapter.
/// Since the primary purpose here is to send this password to the client (so it can log in), the string coming
/// back out of this has to be in plain-text.  That's not great, but it is more or less unavoidable without
/// significant extra effort.
///
/// \return String of the password for the WiFi access point.
String WiFiAdapter::GetPassword(void) { return get_password(); }

/// Pass-through implementation to the sub-class code to set the password to use for the WiFi adapter.
/// Whether the password is plain-text or crypt-text is implementation specific in the sub-code.
///
/// \param password Password to connect to the WiFi access point.
void WiFiAdapter::SetPassword(String const& password) { set_password(password); }

/// Pass-through implementation to the sub-class code to get the IP address associated with the
/// WiFi server.  This is usually set when the server instantiates and the access point is brought up, so this
/// value should only be relied on once the server is actually running.  The value may be cached, however,
/// from previous runs; use caution.
///
/// \return String (dotted notation) of the IP (v4) address of the server on the access point.
String WiFiAdapter::GetServerAddress(void) { return get_address(); }

/// Pass-through implementation to the sub-class code to set the wireless adapter mode.
///
/// \param mode WirelessMode for the adapter on startup
void WiFiAdapter::SetWirelessMode(WirelessMode mode) { return set_wireless_mode(mode); }

/// Pass-through implementation to the sub-class code to get the wireless adapter mode.
///
/// \return WirelessMode enum value, using AP as the default
WiFiAdapter::WirelessMode WiFiAdapter::GetWirelessMode(void) { return get_wireless_mode(); }

/// Pass-through implementation to the sub-class code to return a Stream-version of the client attached to
/// the server, if there is one.
///
/// \return Reference for a WiFiClient upcast to Stream.
Stream& WiFiAdapter::Client(void) { return get_client_stream(); }

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
