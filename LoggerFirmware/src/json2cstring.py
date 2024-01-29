#! @file json2cstring.py
# @brief Generate a C-style string from the JSON configuration data.
#
# The logger firmware embeds a stringified JSON document that contains the default configuration
# for the logger.  On boot, if there isn't a unique ID set, it reads this string, and sets the
# running configuration.  This allows the system to boot from a fresh install of the firmware,
# at least to the stage where you can attach to the WiFi network from the logger and do your own
# install.  This code converts the JSON into the stringified version for inclusion into the C-code
# in Configuration.cpp.
#
# Copyright (c) 2024, University of New Hampshire, Center for Coastal and Ocean Mapping.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# and associated documentation files (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the Software is furnished
# to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import json
with open('boot_config.json') as f:
    config = json.load(f)
cstyle_string = json.dumps(config).replace('"', '\\"')
print(f'static const char *stable_config = \"{cstyle_string}\"')
