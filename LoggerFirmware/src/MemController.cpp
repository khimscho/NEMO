/*!\file MemController.cpp
 * \brief Abstract interface to start the memory system used for log files
 *
 * Depending on the configuration of the logger, the code can write to either an SD card
 * using the SPI interface, or to an eMMC chip using the SDIO/MMC interface.  This object
 * starts the right Arduino interface components for the appropriate system so the main
 * code doesn't have to work this out.
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

#include "MemController.h"
#include "SD.h"
#include "SD_MMC.h"
#include "ParamStore.h"
#include "eMMCController.h"

namespace mem {

const uint8_t default_cs_pin = 5;

class SPIController : public MemController {
public:
    SPIController(uint8_t cs_pin = default_cs_pin)
    : m_csPin(cs_pin)
    {
    }

    ~SPIController(void)
    {
    }

private:
    uint8_t    m_csPin;

    bool start_interface(void)
    {
        return SD.begin(m_csPin);
    }

    void stop_interface(void)
    {
        SD.end();
    }

    fs::FS& get_interface(void)
    {
        return SD;
    }
};

class MMCController : public MemController {
public:
    MMCController(void)
    {
        m_eMMCController = new nemo30::eMMCController();
        m_eMMCController->setModuleStatus(true); // Enable CMD line to module
    }

    ~MMCController(void)
    {
        delete m_eMMCController;
    }

private:
    nemo30::eMMCController  *m_eMMCController;

    bool start_interface(void)
    {
        return SD_MMC.begin();
    }

    void stop_interface(void)
    {
        SD_MMC.end();
    }

    fs::FS& get_interface(void)
    {
        return SD_MMC;
    }
};

MemController *MemControllerFactory::Create(void)
{
    ParamStore *store = ParamStoreFactory::Create();
    String mem_module;
    bool use_spi = true;
    MemController *rc = nullptr;

    if (store->GetKey("MemModule", mem_module)) {
        if (mem_module == "SPI") {
            use_spi = true;
        } else if (mem_module == "MMC") {
            use_spi = false;
        } else {
            Serial.println("ERR: memory module interface not recognised!  Using SPI.");
            use_spi = true;
        }
    }
    if (use_spi) {
        rc = new SPIController();
    } else {
        rc = new MMCController();
    }
    return rc;
}

}