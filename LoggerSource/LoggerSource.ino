/*!\file nmea2000_logger.ino
 * \brief Arduino sketch for the NMEA2000 depth/position logger with network time
 * 
 * This provides the Ardunio-style interface to a NMEA2000 network data logger that's
 * suitable for recording data for Volunteer Geographic Information collection at sea
 * (in keeping with IHO Crowdsourced Bathymetry Working Group recommendations as defined
 * in IHO publication B-12).
 * 
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
 * All Rights Reserved.
 */

#include <SD.h>
#include <NMEA2000_CAN.h> /* This auto-generates the NMEA2000 global for bus control */
#include "N2kLogger.h"
#include "serial_number.h"
#include "SerialCommand.h"
#include "StatusLED.h"

/// Hardware version for the logger implementation (for NMEA2000 declaration)
#define LOGGER_HARDWARE_VERSION "1.0.0"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
const int sd_chip_select_pin = 5; ///< Using VSPI interface for ESP32 SD card (the Arduino default), GPIO5 is SS
#endif

#if defined(__SAM3X8E__)
const int sd_chip_select_pin = 10; ///< For SD card shield on Arduino Due, GPIO10 is SS for the SD
#endif

const unsigned long TransmitMessages[] PROGMEM={0}; ///< List of messages the logger transmits (null set)
const unsigned long ReceiveMessages[] PROGMEM =
  {126992UL /*System Time */,
   127257UL /* Attitude */,
   128267UL /* Water Depth */,
   129026UL /* Course and Speed Over Ground */,
   129029UL /* GNSS Position */,
   130311UL /* Environmental Parameters */,
   130312UL /* Temperature */,
   130313UL /* Humidity */,
   130314UL /* Pressure */,
   130316UL /* Extended Temperature */,
   0
  }; ///< List if messages that the logger expects to receive

nmea::N2000::Logger *N2000Logger = nullptr;     ///< Pointer for NMEA2000 CANbus logger object
nmea::N0183::Logger *N0183Logger = nullptr;     ///< Pointer for serial NMEA data logger object
logger::Manager     *logManager = nullptr;      ///< SD log file manager object
StatusLED           *LEDs = nullptr;			///< Pointer to the status LED manager object
SerialCommand       *CommandProcessor = nullptr;///< Pointer for the command processor object

/// \brief Primary setup code for the logger
///
/// Primary setup for the logger.  This needs to configure the hardware Serial object used
/// for reporting debug information (and taking commands), then setting up the primary objects,
/// then starting the NMEA2000 bus interface.  This should allow messages to start to be
/// received through the CAN bus controller.

void setup()
{
  Serial.begin(115200);
  
  Serial.println("Configuring logger manager ...");
  logManager = new logger::Manager();

  Serial.println("Configuring NEMA2000 logger ...");
  N2000Logger = new nmea::N2000::Logger(&NMEA2000, logManager);
  
  Serial.println("Configuring NMEA0183 logger (and configuring serial ports)...");
  N0183Logger = new nmea::N0183::Logger(logManager);

  Serial.println("Configuring LED indicators ...");
  LEDs = new StatusLED();

  Serial.println("Setting up LED indicators ...");
  LEDs->SetStatus(StatusLED::Status::sINITIALISING);

  Serial.println("Configuring command processor ...");
  CommandProcessor = new SerialCommand(N2000Logger, N0183Logger, logManager, LEDs);

  Serial.println("Initialising SD card interface ...");
  
  /* Set up the SD interface and make sure we have a card */
  if (!SD.begin(sd_chip_select_pin)) {
    // Card is not present, or didn't start ... that's a fatal error
    LEDs->SetStatus(StatusLED::Status::sFATAL_ERROR);
    while (1) {
      LEDs->ProcessFlash(); /* Busy loop to make sure the LED flashes */
      delay(100);
    }
  }
  LEDs->SetStatus(StatusLED::Status::sNORMAL);

  Serial.println("Starting log manager interface to SD card ...");
  logManager->StartNewLog();
  
  Serial.println("Starting NMEA2000 bus interface ...");
  NMEA2000.SetProductInformation(GetSerialNumberString(),
                                1,
                                "Seabed 2030 NMEA2000 Logger",
                                N2000Logger->SoftwareVersion().c_str(),
                                LOGGER_HARDWARE_VERSION
                                );
  NMEA2000.SetDeviceInformation(GetSerialNumber(),
                                130,  /* Device Function: PC gateway */
                                25,   /* Device Class: Inter/Intranetwork Device */
                                2046  /* */
                                );
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 25);
  NMEA2000.EnableForward(false);
  NMEA2000.ExtendTransmitMessages(TransmitMessages);
  NMEA2000.ExtendReceiveMessages(ReceiveMessages);
  NMEA2000.AttachMsgHandler(N2000Logger);

  NMEA2000.Open();
}

/// \brief General processing loop code for the logger.
///
/// General processing loop.  The code needs to service all of the objects that need
/// timely polling, including the NMEA2000 message stack, flashing the status LEDs,
/// if required, and then checking for (and executing) any commands provided by the
/// user either on the hardware Serial link or through the Bluetooth LE UART, if there's
/// a client configured.

void loop()
{
    NMEA2000.ParseMessages();
    N0183Logger->ProcessMessages();
    LEDs->ProcessFlash();
    CommandProcessor->ProcessCommand();
}
