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
from datetime import datetime
from typing import Dict, Any

from wibl.core import getenv


def translate(data: Dict[str,Any], config: Dict[str,Any]) -> Dict[str,Any]:
    """
    Translate from the internal working data dictionary to the GeoJSON structure required by
    DCDB for upload.  This forms the structure for the metadata in addition to re-structuring the
    core data, and fills in any metadata that was stored in the logger, any algorithm information
    and any lineage data.  The current output metadata format is based on IHO Crowd-sourced
    Bathymetry Working Group document B.12 v 3.0.0 (2022-10-01) available at
    https://iho.int/uploads/user/pubs/bathy/B_12_CSB-Guidance_Document-Edition_3.0.0_Final.pdf

    :param data:    Data dictionary from time-interpolation and clean-up
    :type data:     Dict[str,Any] with at least 'depth', 'loggername', 'platform', and 'loggerversion'
    :param config:  Configuration parameters from defaults file for instasll
    :type config:   Dict[str,Any] (see config.py for details)
    :return:        Data dictionary with tags required for conversion to GeoJSON for DCDB
    :rtype:         Dict[str,Any]
    """
    # Original comment was:
    # geojson formatting - Taylor Roy
    # based on https://ngdc.noaa.gov/ingest-external/#_testing_csb_data_submissions example geojson

    feature_lst = []
    
    for i in range(len(data['depth']['z'])):
        timestamp = datetime.fromtimestamp(data['depth']['t'][i]).isoformat()
        
        feature_dict = {
        "type": "Feature",
        "geometry": {
            "type": "Point",
            "coordinates": [
            data['depth']['lon'][i],
            data['depth']['lat'][i]
            ]
        },
        "properties": {
            "depth": data['depth']['z'][i],
            "time": timestamp
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
        "lineage":  [],
        "features": feature_lst
    }
    if data['metadata'] is not None:
        meta = json.loads(data['metadata'])
        if 'platform' in meta:
            final_json_dict['properties']['platform'] = meta['platform']
        if 'providerContactPoint' in meta:
            final_json_dict['properties']['providerContactPoint'] = meta['providerContactPoint']
        if 'depthUnits' in meta:
            final_json_dict['properties']['depthUnits'] = meta['depthUnits']
        if 'timeUnits' in meta:
            final_json_dict['properties']['timeUnits'] = meta['timeUnits']
    
    # The database requires that the unique ID contains the provider's ID, presumably to avoid
    # namespace clashes.  We therefore check now (after the platform metadata is finalised) to make
    # sure that this is the case.
    provider_id = getenv('PROVIDER_ID')
    if provider_id not in final_json_dict['properties']['platform']['uniqueID']:
        final_json_dict['properties']['platform']['uniqueID'] = provider_id + '-' + final_json_dict['properties']['platform']['uniqueID']

    if len(data['algorithms']) > 0:
        final_json_dict['properties']['algorithms'] = data['algorithms']
    if 'lineage' in data:
        final_json_dict['lineage'] = data['lineage']

    return final_json_dict
