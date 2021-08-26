/*!\file HeapMonitor.cpp
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

#include "HeapMonitor.h"

namespace logger {

uint32_t HeapMonitor::HeapSize(void)
{
    return ESP.getHeapSize();
}

uint32_t HeapMonitor::CurrentSize(void)
{
    previous_reported_size = last_reported_size;
    last_reported_size = ESP.getFreeHeap();
    return last_reported_size;
}

uint32_t HeapMonitor::LargestBlock(void)
{
    return ESP.getMaxAllocHeap();
}

int32_t HeapMonitor::DeltaSinceLast(void)
{
    return static_cast<int32_t>(last_reported_size) -
           static_cast<int32_t>(previous_reported_size);
}

uint32_t HeapMonitor::LowWater(void)
{
    return ESP.getMinFreeHeap();
}

void HeapMonitor::FlashMemoryReport(Stream& s)
{
    uint32_t flash_size = ESP.getFlashChipSize();
    uint32_t flash_speed = ESP.getFlashChipSpeed();
    FlashMode_t flash_mode = ESP.getFlashChipMode();
    s.printf("Flash size is %d B, at %d B/s (mode: %d)",
        flash_size, flash_speed, static_cast<uint32_t>(flash_mode));
}

}