/*! \file IMULogger.h
 *  \brief Provide interface to log data from an MPU-6050 6-dof IMU.
 *
 * On the NEMO-30 module, there is an embedded MPU-6050 6-dof IMU.  This class uses
 * the Adafruit MPU-6050 interface to talk to the sensor, and to transfer data to the
 * logManager application so that it gets written into the output data files.
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

#ifndef __IMU_LOGGER_H__
#define __IMU_LOGGER_H__

#include "LogManager.h"
#include "Adafruit_MPU6050.h"

namespace imu {

/// \class Logger
/// \brief Provide interface and logging services for a module's IMU

class Logger {
public:
    Logger(logger::Manager *output);
    ~Logger(void);

    /// \brief Process any data waiting at the IMU into the output log
    void TransferData(void);

    /// \brief Generate a version string for the logger
    String SoftwareVersion(void) const;
    /// \brief Report the version information for the logger
    static void SoftwareVersion(uint16_t& major, uint16_t& minor, uint16_t& patch);

    /// \brief Set debugging status
    void SetVerbose(bool verbose);

private:
    logger::Manager     *m_output;
    bool                m_verbose;
    Adafruit_MPU6050    *m_sensor;
};

}

#endif
