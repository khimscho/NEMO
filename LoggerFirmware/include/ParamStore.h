/*! \file ParamStore.h
 *  \brief Manager for Non-volatile store of parameters for the logger.
 *
 * The logger needs to hold on to some configuration variables for the Bluetooth LE beacon,
 * the WiFi SSID and password, and logging components.  This code manages the resources
 * (depending on the hardware implementation) and provides a factory to make the manager in
 * order to abstract the details.
 *
 * Copyright (c) 2020, University of New Hampshire, Center for Coastal and Ocean Mapping.
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

    /// \brief Set/Reset a binary key value
    bool SetBinaryKey(String const& key, bool value);
    /// \brief Get a value for a binary key value
    bool GetBinaryKey(String const& key, bool& value);
    
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
