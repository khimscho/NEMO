/*! \file eMMCController.h
 *  \brief Provide interface to control eMMC module activity.
 *
 * On the NEMO-30 module, the embedded eMMC module has control lines from the ESP32
 * for enable and reset.  This object provides the software abstraction for this.
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

#ifndef __EMMC_CONTROLLER_H__
#define __EMMC_CONTROLLER_H__

namespace nemo30 {

/// \class eMMCController
/// \brief Provide an abstracted interface to the eMMC module control signals
///
/// On the NEMO-30 implementation of WIBL, the eMMC module needs to have hardware control
/// to enable and reset it.  This class provides that interface.

class eMMCController {
public:
    /// \brief Default constructor
    eMMCController(void);
    /// \brief Default destructor
    ~eMMCController(void);

    /// \brief Control the eMMC module enable pin
    void setModuleStatus(const bool enabled);
    /// \brief Reset the eMMC to a "well know" state
    void resetModule(void);
};

}

#endif
