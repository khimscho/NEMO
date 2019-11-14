/* \file nmea2000_logger.ino
 * \brief Arduino sketch for the NMEA2000 depth/position logger with network time
 * 
 * This provides the Ardunio-style interface to a NMEA2000 network data logger that's
 * suitable for recording data for Volunteer Geographic Information collection at sea
 * (in keeping with IHO Crowdsourced Bathymetry Working Group recommendations as defined
 * in IHO publication B-12).
 * 
 * 2019-08-25.
 */

#include <SD.h>
#include <NMEA2000_CAN.h> /* This auto-generates the NMEA2000 global for bus control */
#include "N2kLogger.h"
#include "serial_number.h"
#include "SerialCommand.h"
#include "StatusLED.h"

#define LOGGER_HARDWARE_VERSION "1.0.0"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
const int sd_chip_select_pin = 5; // Using VSPI interface (the Arduino default), GPIO5 is SS
#else
const int sd_chip_select_pin = 10; // Prototype board used GPIO10 for CS
#endif

const unsigned long TransmitMessages[] PROGMEM={0};
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
  };

N2kLogger     *Logger = NULL;
StatusLED     *LEDs = NULL;
SerialCommand *CommandProcessor = NULL;

void setup()
{
  Serial.begin(115200);

  Serial.println("Configuring logger ...");
  Logger = new N2kLogger(&NMEA2000);

  Serial.println("Configuring LED indicators ...");
  LEDs = new StatusLED();

  Serial.println("Setting up LED indicators ...");
  LEDs->SetStatus(StatusLED::Status::sINITIALISING);

  Serial.println("Configuring command processor ...");
  CommandProcessor = new SerialCommand(Logger, LEDs);

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

  Serial.println("Starting NMEA2000 bus interface ...");
  Logger->StartNewLog();

  NMEA2000.SetProductInformation(GetSerialNumberString(),
                                1,
                                "Seabed 2030 NMEA2000 Logger",
                                Logger->SoftwareVersion().c_str(),
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
  NMEA2000.AttachMsgHandler(Logger);

  NMEA2000.Open();
}

void loop()
{
    NMEA2000.ParseMessages();
    LEDs->ProcessFlash();
    CommandProcessor->ProcessCommand();
}
