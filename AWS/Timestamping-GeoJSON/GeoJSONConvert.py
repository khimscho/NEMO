## \file GeoJSONConvert.py
# \brief Convert the interpolated data from the logger into correctly formatted GeoJSON data for DCDB
#
# The data logger provides data in binary format, and the Timestamping.py module reads that format
# (using LoggerFile.py) and then fixes the timestamps, end up with a dictionary that contains the
# reference times for each depth measurement, along with the depth in metres and position in latitude
# and logitude.  The code here converts this to the GeoJSON format required by DCDB for submission.
#
# This code is based on work done by Taylor Roy et al., as part of a UNH Computer Science undergraduate
# programme final year project, and then modified for slightly better maintainability.
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

def translate(data):
    # Original comment was:
    # geojson formatting - Taylor Roy
    # based on https://ngdc.noaa.gov/ingest-external/#_testing_csb_data_submissions example geojson

    feature_lst = []
    
    for i in range(len(data['z'])):
        feature_dict = {
        "type": "Feature",
        "geometry": {
            "type": "Point",
            "coordinates": [
            data['lon'][i],
            data['lat'][i]
            ]
        },
        "properties": {
            "depth": data['z'][i],
            "time": data['t'][i]
        }
        }

        feature_lst.append(dict(feature_dict))


    final_json_dict = {
        "type": "FeatureCollection",
        "crs": {
            "type": "name",
            "properties": {
                "name": "EPSG:4326"
            }
        },
        "properties": {
            "convention": "CSB 2.0",
            "platform": {
                "uniqueID": data['loggername'],
                "type": "Ship",
                "name": data['platform'],
                "IDType": "LoggerName",
                "IDNumber": data['loggername']
            },
            "providerContactPoint": {
                "orgName": "CCOM/JHC, UNH",
                "email": "wibl@ccom.unh.edu",
                "logger": "WIBL",
                "loggerVersion": data['loggerversion']
            },
            "depthUnits": "meters",
            "timeUnits": "ISO 8601"
        },
        "features": feature_lst
    }
    if data['metadata'] is not None:
        final_json_dict['properties']['platform'] = json.loads(data['metadata'])
    if len(data['algorithms']) > 0:
        final_json_dict['properties']['processing'] = data['algorithms']

    return final_json_dict
