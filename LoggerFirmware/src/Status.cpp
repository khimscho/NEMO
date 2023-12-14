
#include "ArduinoJson.h"
#include "LogManager.h"
#include "Configuration.h"
#include "N0183Logger.h"
#include "N2kLogger.h"
#include "IMULogger.h"
#include "SerialCommand.h"
#include "DataMetrics.h"

namespace logger {
namespace status {

/// Generate a list of all known files on the logger (and their attributes) into
/// an existing JSON document.  This includes the number of files, their reference
/// IDs, sizes, MD5 hashes, and filenames in the store.
///
/// @param filelist JsonDocument with the file list, ready to insert

DynamicJsonDocument GenerateFilelist(logger::Manager *m)
{
    uint32_t filenumbers[logger::MaxLogFiles];
    uint32_t n_files = m->CountLogFiles(filenumbers);
    DynamicJsonDocument doc(100*n_files); // Approximate guess, but can expand

    doc["files"]["count"] = n_files;
    for (int n = 0; n < n_files; ++n) {
        String filename;
        uint32_t filesize;
        logger::Manager::MD5Hash filehash;
        uint16_t uploadCount;
        m->EnumerateLogFile(filenumbers[n], filename, filesize, filehash, uploadCount);
        StaticJsonDocument<256> entry;
        entry["id"] = filenumbers[n];
        entry["len"] = filesize;
        if (!filehash.Empty())
            entry["md5"] = filehash.Value();
        entry["url"] = filename;

        if (!doc["files"]["detail"].add(entry.as<JsonObject>())) {
            // Out of memory, so make a new allocation
            int capacity = doc.capacity() * 2;
            DynamicJsonDocument new_doc(capacity);
            new_doc.set(doc);
            doc = new_doc;
            doc["files"]["detail"].add(entry.as<JsonObject>());
        }
    }
    return doc;
}

DynamicJsonDocument CurrentStatus(logger::Manager *m)
{
    DynamicJsonDocument status(10240);

    status["version"]["firmware"] = logger::FirmwareVersion();
    status["version"]["commandproc"] = SerialCommand::SoftwareVersion();
    status["version"]["nmea0183"] = nmea::N0183::Logger::SoftwareVersion();
    status["version"]["nmea2000"] = nmea::N2000::Logger::SoftwareVersion();
    status["version"]["imu"] = imu::Logger::SoftwareVersion();
    status["version"]["serialiser"] = Serialiser::SoftwareVersion();

    int now = millis();
    status["elapsed"] = now;

    String server_status, boot_status;
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_WS_STATUS_S, server_status);
    logger::LoggerConfig.GetConfigString(logger::Config::CONFIG_WS_BOOTSTATUS_S, boot_status);
    status["webserver"]["current"] = server_status;
    status["webserver"]["boot"] = boot_status;

    status["data"] = logger::Metrics.LastKnownGood();

    DynamicJsonDocument filelist(GenerateFilelist(m));
    status["files"] = filelist["files"];

    return status;
}

}
}
