##\file run_tests.py
# \brief Simple (and non-complete) testing for REST API for database management
#
# This code is intended to be used primarily to exercise the database REST API during
# development, and does not constitute a complete regression test environment for
# the system.  It will, however, do interesting things to the database, which should
# allow for new systems to be checked.
#
# Copyright 2023 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
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

import requests
import uuid

from return_codes import ReturnCodes, ProcessingStatus, UploadStatus

BASE_URI = 'http://127.0.0.1:5000/'
filename = 'cddb-aefga-weqa'

def generate_random_entries():
    n_raw_added = 0
    n_geo_added = 0
    for f in range(100):
        fileid = str(uuid.uuid4())
        response = requests.post(BASE_URI + 'wibl/' + fileid, json={'size': 10.4})
        if response.status_code != ReturnCodes.RECORD_CREATED.value:
            print(f'error: failed to generate {fileid} in WIBL database.')
        else:
            n_raw_added += 1
        response = requests.put(BASE_URI + 'wibl/' + fileid,
                                json={'logger': 'UNHJHC-wibl-1', 'platform': 'USCGC Healy',
                                    'observations': 100232, 'soundings': 8023,
                                    'startTime': '2023-01-23T12:34:45.142',
                                    'endTime': '2023-01-24T01:45:23.012',
                                    'status': ProcessingStatus.PROCESSING_SUCCESSFUL.value})
        if response.status_code != ReturnCodes.RECORD_CREATED.value:
            print(f'error: failed to update record from WIBL file {fileid}.')
        geojson_fileid = fileid + '.json'
        response = requests.post(BASE_URI + 'geojson/' + geojson_fileid, json={'size': 5.4})
        if response.status_code != ReturnCodes.RECORD_CREATED.value:
            print(f'error: failed to generate {geojson_fileid} in GeoJSON database.')
        else:
            n_geo_added += 1
        response = requests.put(BASE_URI + 'geojson/' + geojson_fileid,
                                json={'logger': 'UNHJHC-wibl-1',
                                    'soundings': 8023,
                                    'status': UploadStatus.UPLOAD_SUCCESSFUL.value})
        
    print(f'Generated {n_raw_added} random entries into database for WIBL files, and {n_geo_added} for GeoJSON equivalents.')

response = requests.delete(BASE_URI + 'wibl/' + filename)
print(response.json())
response = requests.post(BASE_URI + 'wibl/' + filename, json={ 'size': 10.4 })
print(response.json())
response = requests.put(BASE_URI + 'wibl/' + filename, json={ 'size': 5.4, 'observations': 10232, 'logger': 'UNHJHC-wibl-1', 'startTime': '2023-01-23T13:45:08.231', 'status': 1})
print(response.json())
response = requests.delete(BASE_URI + 'wibl/' + filename)
print(response.json())

response = requests.delete(BASE_URI + 'geojson/' + filename)
print(response.json())
response = requests.post(BASE_URI + 'geojson/' + filename, json={ 'size': 5.4 })
print(response.json())
response = requests.put(BASE_URI + 'geojson/' + filename, json={ 'logger': 'UNHJHC-wibl-1', 'soundings': 8020, 'size': 5.3, 'status': 1 })
print(response.json())
response = requests.delete(BASE_URI + 'geojson/' + filename)
print(response.json())

#generate_random_entries()

response = requests.get(BASE_URI + 'wibl/all')
print(response.json())

response = requests.get(BASE_URI + 'geojson/all')
print(response.json())
