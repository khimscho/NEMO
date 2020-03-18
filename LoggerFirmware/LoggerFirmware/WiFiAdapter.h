/*! \file WiFiAdapter.h
 *  \brief Interface for the WiFi adapter on the module, and a factory to abstract the object
 *
 * In order to get to a decent speed for data transfer, we need to use the WiFi interface rather
 * than the BLE.  This provides the interface to abstract the details of this away.
 *
 * Copyright (c) 2020, University of New Hampshire, Center for Coastal and Ocean Mapping
 * & NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#ifndef __WIFI_ADAPTER_H__
#define __WIFI_ADAPTER_H__

#include <WiFi.h>

/// \class WiFiAdapter
/// \brief Abstract base class for WiFi access.

class WiFiAdapter {
public:
    /// \brief Default destructor to allow for sub-classes.
    virtual ~WiFiAdapter(void) {}
    
    /// \brief Start the WiFi adapter and access point.
    bool Startup(void);
    /// \brief Shut down the WiFi adapter and access point.
    void Shutdown(void);
    
    /// \brief Test whether there is a client connected to the WiFi adapter (and consequently that it's up and working).
    bool IsConnected(void);
    /// \brief Test whether there is currently data available in the client buffers.
    bool DataAvailable(void);
    /// \brief Extract the first string from the WiFi client (usually a command string).
    String ReceivedString(void);
    
    /// \brief Manage the transfer of a log file to the client, given the filename.
    bool TransferFile(String const& filename);
    
    /// \brief Return the SSID for the WiFi adapter (stored in NVRAM).
    String GetSSID(void);
    /// \brief Set the SSID for the WiFi adapter (stored in NVRAM).
    void SetSSID(String const& ssid);
    /// \brief Return the password for the WiFi adapter (stored in NVRAM).
    String GetPassword(void);
    /// \brief Set the password for the WiFi adapter (stored in NVRAM).
    void SetPassword(String const& password);
    /// \brief Return a string version of the IP address in use for the server.
    String GetServerAddress(void);
    
    /// \brief Return a reference to the stream associated with the client TCP/IP socket (if available).
    Stream& Client(void);
    
private:
    /// \brief Sub-class implementation of code to start the interface.
    virtual bool start(void) = 0;
    /// \brief Sub-class implementation of code to stop the interface.
    virtual void stop(void) = 0;
    
    /// \brief Sub-class implementation of code to check client-connect status.
    virtual bool isConnected(void) = 0;
    /// \brief Sub-class implementation of code to check on whether data's waiting.
    virtual bool dataCount(void) = 0;
    /// \brief Sub-class implementation of code to read the first string from the buffer.
    virtual String readBuffer(void) = 0;
    
    /// \brief Sub-class implementation of code to send a log file to the client.
    virtual bool sendLogFile(String const& filename) = 0;
    
    /// \brief Sub-class implementation to set the SSID for the WiFi.
    virtual void set_ssid(String const& ssid) = 0;
    /// \brief Sub-class implementation to get the SSID being used for the WiFi.
    virtual String get_ssid(void) = 0;
    /// \brief Sub-class implementation to set the password to be used for the WiFi.
    virtual void set_password(String const& password) = 0;
    /// \brief Sub-class implementation to get the password to be used for the WiFi.
    virtual String get_password(void) = 0;
    /// \brief Sub-class implementation to set the IP (v4) address being used for the adapter.
    virtual void set_address(IPAddress const& address) = 0;
    /// \brief Sub-class to get the IP (v4) address being used by the adapter.
    virtual String get_address(void) = 0;
    
    /// \brief Sub-class implementation of code to get a Stream reference for the client, if there is one.
    virtual Stream& get_client_stream(void) = 0;
};

/// \class WiFiAdapterFactory
/// \brief Factory object to generate a WiFiAdapter for the particular hardware module in use

class WiFiAdapterFactory {
public:
    /// \brief Constructor for a WiFiAdapter of the appropriate implementation.
    static WiFiAdapter *Create(void);
};

#endif
