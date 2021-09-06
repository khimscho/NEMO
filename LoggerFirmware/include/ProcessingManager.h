/*! \file ProcessingManager.h
 *  \brief Processing algorithm name/parameter storage and retrieval
 *
 * The logger can be told which processing algorithms (in addition to timestamping and format
 * conversion) need to be run on the data (e.g., de-duplication of depth values, spike removal,
 * reputation assessment, etc.) so that the cloud process doesn't have to do monster lookups for
 * this from all of the different loggers.  This module provides a unified interface to store
 * these for the logger, and the means to write this into the output log files.
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping.
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

#ifndef __PROCESSING_MANAGER_H__
#define __PROCESSING_MANAGER_H__

#include "Arduino.h"
#include "serialisation.h"
#include "MemController.h"

namespace logger {

/// \class ProcessingManager
/// \brief Hold information on algorithms that need to be run on the data in the cloud
class ProcessingManager {
public:
    ProcessingManager(void);
    ~ProcessingManager(void);

    void AddAlgorithm(String const& alg_name, String const& alg_params);
    void ListAlgorithms(Stream& s);
    void SerialiseAlgorithms(Serialiser *s);

private:
    String  m_algorithms;
    String  m_parameters;
};

}

#endif
