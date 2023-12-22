#include "ArduinoJson.h"
#include "LogManager.h"

namespace logger {
namespace status {

/// @brief Generate a JSON list of the set of files (and their metrics) currently on the logger
DynamicJsonDocument GenerateFilelist(logger::Manager *m);

/// @brief Generate a JSON document representing the current status of the logger
DynamicJsonDocument CurrentStatus(logger::Manager *m);

/// @brief Generate a correctly-sized JSON document from a minified string
DynamicJsonDocument GenerateJSON(String const& s);

}
}
