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
#include "LSM6DSL.h"
#include "IMULogger.h"

namespace imu {

const int SoftwareVersionMajor = 1; ///< Software major version for the logger
const int SoftwareVersionMinor = 0; ///< Software minor version for the logger
const int SoftwareVersionPatch = 0; ///< Software patch version for the logger

const int IMUInterruptPin = 39; // Interrupt from LSM6DSL for data ready
const int IMUAddressI2C = 0x6A; // Address on I2C with address pin grounded

// LSM6DSL Registers not defined in the support library
const uint8_t LSM6DSL_DRDY_PULSE_CFG = 0x0B;    // Control for latched/pulsed interrupt
const uint8_t LSM6DSL_CTRL3_C = 0x12;           // Control register 3
const uint8_t LSM6DSL_MASTER_CONFIG = 0x1A;     // Master configuration register
const uint8_t LSM6DSL_STATUS_REGISTER = 0x1E;   // Status register for data ready

// INT1 enabling bits for INT1_CTRL
const uint8_t LSM6DSL_INT1_DRDY_G = 0x01;
const uint8_t LSM6DSL_INT2_DRDY_XL = 0x02;

// DRDY_PULSE_CFG register (latch/pulse)
const uint8_t LSM6DSL_DRDY_PULSED = 0x80; // 0 = latched mode (until read), 1 = 75us pulses

// CTRL3 register bits
const uint8_t LSM6DSL_CTRL3_HLACTIVE = 0x20; // 0 = active high, 1 = active low
const uint8_t LSM6DSL_CTRL3_PP_OD = 0x10; // 0 = push-pull, 1 = open drain

// Master configuration register controls
const uint8_t LSM6DSL_INT1_DRDY_EN = 0x80; // 0 = disable, 1 = enable

volatile bool imu_data_ready = false;

/// Interrupt service routine for the IMU interrupt, indicating that data is ready at the IMU
/// for reading.  This indicates that the data is ready by setting a global variable that the
/// logger can read.

void IRAM_ATTR IMUDataReady()
{
    imu_data_ready = true;
}

/// Standard constructor for the IMU logger, with output to the specified log manager.
///
/// \param output   Pointer to the log manager to use for output
/// \return N/A

Logger::Logger(logger::Manager *output)
: m_output(output), m_verbose(false)
{
    m_sensor = new LSM6DSL(LSM6DSL_MODE_I2C, IMUAddressI2C);
    
    // After construction of the instance, we can change configuration before the begin()
    m_sensor->settings.gyroRange = LSM6DSL_ACC_GYRO_FS_G_2000dps;       // Full scale degrees per second (must be from known list)
    m_sensor->settings.gyroSampleRate = LSM6DSL_ACC_GYRO_ODR_G_104Hz;   // Sampling rate, Hz (must be from known list)
    m_sensor->settings.accelRange = LSM6DSL_ACC_GYRO_FS_XL_4g;          // Full scale in Gs (must be from known list)
    m_sensor->settings.accelSampleRate = LSM6DSL_ACC_GYRO_ODR_XL_104Hz; // Sampling rate, Hz (must be from known list)

    if (m_sensor->begin() != IMU_SUCCESS) {
        Serial.println("Failed to initialise LSM6DSL; logging disabled.");
        delete m_sensor;
        m_sensor = nullptr;
        m_output = nullptr;
    } else {
        // Attach an interrupt handler before setting up the external system to cause them
        /*pinMode(IMUInterruptPin, INPUT);
        attachInterrupt(digitalPinToInterrupt(IMUInterruptPin), IMUDataReady, FALLING);*/

        // Set up for interrupts so that we don't have to poll for data availability
        /*m_sensor->writeRegister(LSM6DSL_ACC_GYRO_INT1_CTRL, LSM6DSL_INT1_DRDY_G);
        m_sensor->writeRegister(LSM6DSL_CTRL3_C, LSM6DSL_CTRL3_HLACTIVE | LSM6DSL_CTRL3_PP_OD);
        m_sensor->writeRegister(LSM6DSL_DRDY_PULSE_CFG, LSM6DSL_DRDY_PULSED);
        m_sensor->writeRegister(LSM6DSL_MASTER_CONFIG, LSM6DSL_INT1_DRDY_EN);*/
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

/// Test the data availability register in the IMU for new data.
///
/// \return True if new data is available, otherwise false

bool Logger::data_available(void)
{
    uint8_t status;
    if (m_sensor->readRegister(&status, LSM6DSL_STATUS_REGISTER) != IMU_SUCCESS) return false;
    return (status & 0x7) != 0;
}

float Logger::convert_acceleration(int16_t v)
{
    return (float)v * m_sensor->settings.accelRange / 32767.0;
}

float Logger::convert_gyrorate(int16_t v)
{
    return (float)v * m_sensor->settings.gyroRange / 32767.0;
}

float Logger::convert_temperature(int16_t t)
{
    return (float)t / 256.0 + 25.0;
}

/// Get the next sensor reading from the IMU, and write into the output stream.
///
/// \return N/A

void Logger::TransferData(void)
{
    if (m_sensor == nullptr) return;

    int16_t ax, ay, az, gx, gy, gz, t;
    unsigned long now = millis();

    if (data_available()) {
        ax = m_sensor->readRawAccelX();
        ay = m_sensor->readRawAccelY();
        az = m_sensor->readRawAccelZ();
        gx = m_sensor->readRawGyroX();
        gy = m_sensor->readRawGyroY();
        gz = m_sensor->readRawGyroZ();
        t = m_sensor->readRawTemperature();
        Serial.printf("DBG: a = (%X, %X, %X) g = (%X, %X, %X) t = %X\n", ax, ay, az, gx, gy, gz, t);
        Serialisable buffer(7*sizeof(float) + sizeof(unsigned long));
        buffer += static_cast<uint32_t>(now);
        buffer += convert_acceleration(ax);
        buffer += convert_acceleration(ay);
        buffer += convert_acceleration(az);
        buffer += convert_gyrorate(gx);
        buffer += convert_gyrorate(gy);
        buffer += convert_gyrorate(gz);
        buffer += convert_temperature(t);
        m_output->Record(logger::Manager::PacketIDs::Pkt_LocalIMU, buffer);

        imu_data_ready = false;
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