/*! \file IMULogger.cpp
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

#include "serialisation.h"
#include "LogManager.h"
#include "Adafruit_MPU6050.h"
#include "IMULogger.h"

namespace imu {

const int SoftwareVersionMajor = 1; ///< Software major version for the logger
const int SoftwareVersionMinor = 0; ///< Software minor version for the logger
const int SoftwareVersionPatch = 0; ///< Software patch version for the logger

/// Standard constructor for the IMU logger, with output to the specified log manager.
///
/// \param output   Pointer to the log manager to use for output
/// \return N/A

Logger::Logger(logger::Manager *output)
: m_output(output), m_verbose(false)
{
    m_sensor = new Adafruit_MPU6050();
    m_sensor->setAccelerometerRange(MPU6050_RANGE_8_G);
    m_sensor->setGyroRange(MPU6050_RANGE_500_DEG);
    m_sensor->setFilterBandwidth(MPU6050_BAND_21_HZ);
    m_sensor->setCycleRate(MPU6050_CYCLE_20_HZ);
    if (!m_sensor->begin()) {
        Serial.println("Failed to initialise MPU-6050; logging disabled.");
        delete m_sensor;
        m_sensor = nullptr;
        m_output = nullptr;
    }
}

/// Standard destructor.  This removes the sensor interface object, hopefully also
/// turning the sensor off.
///
/// \return N/A

Logger::~Logger(void)
{
    delete m_sensor;
}

/// Get the next sensor reading from the IMU, and write into the output stream.
///
/// \return N/A

void Logger::TransferData(void)
{
    if (m_sensor == nullptr) return;

    sensors_event_t acc, gyro, temp;
    if (m_sensor->getEvent(&acc, &gyro, &temp)) {
        Serialisable buffer(28);
        buffer += acc.acceleration.x;
        buffer += acc.acceleration.y;
        buffer += acc.acceleration.z;
        buffer += gyro.gyro.x;
        buffer += gyro.gyro.y;
        buffer += gyro.gyro.z;
        buffer += temp.temperature;
        m_output->Record(logger::Manager::PacketIDs::Pkt_LocalIMU, buffer);
    }
}

/// Assemble a logger version string
///
/// \return Printable version of the version information

String Logger::SoftwareVersion(void) const
{
    String rtn;
    rtn = String(SoftwareVersionMajor) + "." + String(SoftwareVersionMinor) +
            "." + String(SoftwareVersionPatch);
    return rtn;
}

/// Report the versioning information for the logger, primarily so that it can be written
/// into the log file to allow the reader to adapt to the contents during read.
///
/// \param major    Major software version
/// \param minor    Minor software version
/// \param patch    Patch level/build level

void Logger::SoftwareVersion(uint16_t& major, uint16_t& minor, uint16_t& patch)
{
    major = SoftwareVersionMajor;
    minor = SoftwareVersionMinor;
    patch = SoftwareVersionPatch;
}

/// Set the reporting information 
void Logger::SetVerbose(bool verbose)
{
    m_verbose = verbose;
}

}