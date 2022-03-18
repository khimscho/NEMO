/*! \file IMULogger.h
 *  \brief Provide interface to log data from an LSM6DS3 6-dof IMU.
 *
 * On the NEMO-30 module, there is an embedded LSM6DS3 6-dof IMU.  This class uses
 * the Adafruit LSM6DS3 interface to talk to the sensor, and to transfer data to the
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
#include "LSM6DSL.h"

namespace imu {

/// \class Logger
/// \brief Provide interface and logging services for a module's IMU
///
/// On some versions of WIBL loggers, there is a small motion sensor that can be used to provide
/// some information on motion.  This class provides the interface to the sensor, although only
/// to the extent of telling the system to pull data from the sensor and write it to SD card, if
/// any data is ready for transfer.
///     Since it's possible that you can get replicated data on the stream (if it's converting slower
/// than you're reading), so the algorithm here keeps track of the last acceleration values that
/// were written to the SD card and only writes new data if any of the three axes are different.

class Logger {
public:
    /// \brief Default constructor, for data to a given log manager
    Logger(logger::Manager *output);
    /// \brief Default destructor
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
    logger::Manager     *m_output;      ///< Pointer to the log manager to use for reporting data
    bool                m_verbose;      ///< Flag: True => write more data about operations, False => quiet mode
    LSM6DSL             *m_sensor;      ///< Pointer to the motion sensor interface library

    bool data_available(void);
    float convert_acceleration(int16_t v);
    float convert_gyrorate(int16_t v);
    float convert_temperature(int16_t t);
};

}

#endif
