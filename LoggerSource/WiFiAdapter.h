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

/// \class WiFiAdapter
/// \brief Abstract base class for WiFi access

class WiFiAdapter {
public:
    virtual ~WiFiAdapter(void) {}
    
    bool Startup(void);
    void Shutdown(void);
    
    String GetSSID(void);
    void SetSSID(String const& ssid);
    String GetPassword(void);
    void SetPassword(String const& password);
    
private:
    virtual bool start(void) = 0;
    virtual void stop(void) = 0;
    virtual void set_ssid(String const& ssid) = 0;
    virtual String get_ssid(void) = 0;
    virtual void set_password(String const& password) = 0;
    virtual String get_password(void) = 0;
};

/// \class WiFiAdapterFactory
/// \brief Factory object to generate a WiFiAdapter for the particular hardware module in use

class WiFiAdapterFactory {
public:
    static WiFiAdapter *Create(void);
};

#endif
