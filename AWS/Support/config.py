## \file config.py
# \brief Handle configuration files
#
# The cloud processing code needs a number of external parameters to work out where to find the data,
# where to store intermediate files, and so on.  These are summarised in a JSON configuration file that
# should be installed in the same directory as the serverless handler.  The code here handles reading
# this configuration into a dictionary, and then doing any preliminary processing required to get the
# configuration ready for use (e.g., converting strings to actual values, pre-computing values from
# raw inputs, etc.).
#
# Copyright 2021 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
# Hydrographic Center, University of New Hampshire.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.

import json
from typing import Dict, Any

class BadConfiguration(Exception):
    pass

def read_config(config_file: str) -> Dict[str, Any]:
    """There are a number of configuration parameters for the algorithm, such as the provider ID name
       to write into the metadata, and use for filenames; where to stage the intermediate GeoJSON files
       before they're uploaded to the database; and where to send the files.  There are also parameters
       to control the local algorithm, including:
    
            elapsed_time_width:     Width (bits) of the elapsed time data in the WIBL file (usu. 32)
            verbose:                Boolean (text) for verbose reporting of processing (usu. False)
            local:                  Boolean (text) for local testing (usu. False for cloud deployment)
       
       This code reads the JSON file with these parameters, and does appropriate translations to them so
       that the rest of the code can just read from the resulting dictionary.
    """
    with open(config_file) as f:
        config = json.load(f)
        
    # We need to tell the timestamping code what the roll-over size is for the elapsed times
    # stored in the WIBL file.  This allows it to determine when to add a day to the count
    # so that we don't wrap the interpolation.
    config['elapsed_time_quantum'] = 1 << config['elapsed_time_width']
    
    return config

