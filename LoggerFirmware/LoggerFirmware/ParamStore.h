/*! \file ParamStore.h
 *  \brief Manager for Non-volatile store of parameters for the logger.
 *
 * The logger needs to hold on to some configuration variables for the Bluetooth LE beacon,
 * the WiFi SSID and password, and logging components.  This code manages the resources
 * (depending on the hardware implementation) and provides a factory to make the manager in
 * order to abstract the details.
 *
 * Copyright (c) 2020, University of New Hampshire, Center for Coastal and Ocean Mapping
 * & NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#ifndef __PARAM_STORE_H__
#define __PARAM_STORE_H__

#include <Arduino.h>

/// \class ParamStore
/// \brief Abstract interface for the parameter store key-value object

class ParamStore {
public:
    /// \brief Default destructor so that other sub-classes can make their own
    virtual ~ParamStore() {}
    /// \brief Set/Reset a key-value pair
    bool SetKey(String const& key, String const& value);
    /// \brief Get a value for a given key
    bool GetKey(String const& key, String& value);
    
private:
    /// \brief Sub-class implementation of the mechanics to set a key to a value.
    virtual bool set_key(String const& key, String const& value) = 0;
    /// \brief Sub-class implementation of the mecahnics to get a value for a key.
    virtual bool get_key(String const& key, String& value) = 0;
};

/// \class ParamStoreFactory
/// \brief Provide a a factory method for the parameter store access object

class ParamStoreFactory {
public:
    /// \brief Create an implementation of a ParamStore appropriate for the current hardware.
    static ParamStore *Create(void);
};

#endif
