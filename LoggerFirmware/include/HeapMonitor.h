/*!\file HeapMonitor.h
 * \brief Helper class to monitor heap memory usage
 *
 * The memory available to the ESP32 can be quite small, and therefore it is important
 * to understand what each component of the system is using for dynamic objects to
 * optimise for this.  This module provides a small object that can be used to interrogate
 * the runtime to provide this information.
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

#ifndef __HEAP_MONITOR_H__
#define __HEAP_MONITOR_H__

#include <stdint.h>
#include <Arduino.h>

namespace logger {

/// \class HeapMonitor
/// \brief Simple interface to the Arduino run-time monitoring

class HeapMonitor {
public:
    HeapMonitor(void) {}

    uint32_t HeapSize(void); ///< Maximum size of the heap (static)

    uint32_t CurrentSize(void);     ///< Current free heap (bytes)
    uint32_t LargestBlock(void);    ///< Largest block that can be allocated (bytes)
    int32_t  DeltaSinceLast(void);  ///< Change in heap free since last CurrentSize() (bytes)
    uint32_t LowWater(void);        ///< Minimum heap size (bytes)

    void FlashMemoryReport(Stream& s); ///< Report static configuration of flash memory to stream

private:
    uint32_t last_reported_size;
    uint32_t previous_reported_size;
};

}

#endif
