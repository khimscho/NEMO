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
///
/// The memory space in the logger is generally quite limited, and therefore we need to be
/// cognisant of the costs involved in instantiating certain objects or libraries, since too
/// little heap memory can cause instability.  This class provides a wrapper around the hardware's
/// mechanism to read the free space in the heap, and to report on the size and configuration
/// of the flash memory on the microcontroller module, if any.
///     As an aid to determining how much memory is being used by each library or object being
/// used, the code will also keep track of the size of the free space in the heap at each call to
/// CurrentSize(), and report the difference through DeltaSinceLast().  Getting this to work
/// reliably requires the code to be disciplined about always calling CurrentSize() after any
/// significant memory operation, or group of operations.

class HeapMonitor {
public:
    /// \brief Default constructor
    HeapMonitor(void) {}

    /// \brief Report the maximum size of the heap (a static)
    uint32_t HeapSize(void);

    /// \brief Report the current free space in the heap (bytes)
    uint32_t CurrentSize(void);
    /// \brief Report the largest allocable block in the heap (bytes)
    uint32_t LargestBlock(void);
    /// \brief Change in heap free space between last two CurrentSize() calls (bytes)
    int32_t  DeltaSinceLast(void);
    /// \brief Minimum size of free heap space since first boot (bytes)
    uint32_t LowWater(void);

    /// \brief Report a text description of the logger's flash memory, if available
    void FlashMemoryReport(Stream& s);

private:
    uint32_t last_reported_size;        ///< Free space in heap at last CurrentSize() call
    uint32_t previous_reported_size;    ///< Free space in heap at penultimate CurrentSize() call
};

}

#endif
