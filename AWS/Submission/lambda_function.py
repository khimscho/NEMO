## \file lambda_function.py
#  \brief AWS Lambda to transfer a processed (GeoJSON) S3 file to DCDB
#
# This is designed to run as an AWS Lambda, responding to a file appearing in a given
# S3 bucket.  The script pulls the file locally, then transmits to DCDB via their
# submission API.
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
import requests
import boto3
from urllib.parse import unquote_plus
from typing import Dict, Any

# Local modules
import config as conf
import datasource as ds

s3 = boto3.resource('s3')

## Helper function to read a local file with an AWS Lambda event in JSON format
#
# For local testing of the code, you need to provide a simulated Lambda event.  This helper
# function can be used to parse a JSON file (format as for the Lambda, best downloaded from
# AWS directly) and ready it for use in the handler.
#
# \param event_file Filename for the local file to read and parse
# \return JSON dictionary from the specified file
def read_local_event(event_file: str) -> Dict:
    """For local testing, you need to simulate an AWS Lambda event stucture to give the code something
       to do.  You can download the JSON for a typical event from the AWS Lambda console and store it
       locally.  This helper function can be called interactively to load this information.
    """
    with open(event_file) as f:
        event = json.load(f)
    return event

## Send a specified GeoJSON dictionary to the user's specified end-point
#
# Manage the process for formatting the POST request to DCDB's submission API, transmitting the file into
# the DCDB incoming buffers.  Among other things, this makes sure that the provider identification is
# embedded in the unique identifier (since DCDB uses this to work out where to route the data).  A local
# mode is also provided so you can test the upload without having to be connected, or annoy DCDB.
#
# \param source_uniqueID    Unique ID encoded into the GeoJSON metadata
# \param provider_id        DCDB recognition code for the person authorising the upload
# \param provider_auth      DCDB authorisation key for upload
# \param local_file         Filename for the local GeoJSON file to transmit
# \param config             Configuration dictionary
# \return True if the upload succeeded, otherwise False

def transmit_geojson(source_uniqueID: str, provider_id: str, provider_auth: str, local_file: str, config: Dict[str,Any]) -> bool:
    headers = {
        'x-auth-token': provider_auth
    }
    # The unique ID that we provide to DCDB must be preceeded by our provider ID and a hyphen.
    # This should happen before the code gets to here, but just in case, we enforce this requirement.
    if provider_id not in source_uniqueID:
        dest_uniqueID = provider_id + '-' + source_uniqueID
    else:
        dest_uniqueID = source_uniqueID

    if config['verbose']:
        print(f'Source ID is: {source_uniqueID}; ' + 
              f'Destination object uniqueID is: {dest_uniqueID}; ' +
              f'Authorisation token is: {provider_auth}')

    files = {
        'file': (local_file, open(local_file, 'rb')),
        'metadataInput': (None, '{\n    "uniqueID": "' + dest_uniqueID + '"\n}')
    }
    
    if config['local']:
        upload_point = config['upload_point']
        print(f'Local mode: Transmitting to {upload_point} for source ID {source_uniqueID} with destination ID {dest_uniqueID}.')
        rc = True
    else:
        upload_url = config['upload_point']
        if config['verbose']:
            print(f'Transmitting for source ID {source_uniqueID} to {upload_url} as destination ID {dest_uniqueID}.')
        response = requests.post(upload_url, headers=headers, files=files)
        json_response = response.json()
        print(f'POST response is {json_response}')
        try:
            rc = json_response['success']
            if not rc:
                rc = None
        except json.decoder.JSONDecodeError:
            rc = None

    return rc

def lambda_handler(event, context):
    try:
        config = conf.read_config('configure.json')
    except conf.BadConfiguration:
        return {
            'statusCode':   400,
            'body':         'Bad configuration file.'
            }
    
    # We need to find the provider's authorisation ID and identifier so that we can
    # annotate the call to the DCDB upload point.  These are stored in a JSON file in
    # the same directory as the Lambda source, with a well-known name
    with open('credentials.json') as f:
        creds = json.load(f)
    
    # Construct a data source to handle the input items, and a controller to manage the transmission of
    # data from buckets to local files.  We instanatiate the DataSource and CloudController sub-classes
    # explicitly, since this code is also specific to AWS.
    source = ds.AWSSource(event, config)
    controller = ds.AWSController(config)
    
    # Loop through all of the source files in the incoming bucket, attempting to upload each in turn, and
    # keeping track of those that succeed and fail.
    item = source.nextSource()
    succeeded = []
    failed = []
    while item is not None:
        (local_file, source_id) = controller.obtain(item)
        if source_id is None:
            if config['verbose']:
                print(f'Failed to transfer item {item} because it has no SourceID specified.')
            failed.append(item)
        else:
            rc = transmit_geojson(source_id, creds['provider_id'], creds['provider_auth'], local_file, config)
            if rc is None:
                if config['verbose']:
                    print(f'Failed to transfer item {item}')
                failed.append(item)
            else:
                if config['verbose']:
                    print(f'Succeeded in transferring item {item}')
                succeeded.append(item)
        item = source.nextSource()
    
    # Status report for the user
    if failed:
        status_code = 400
    else:
        status_code = 200
    n_succeed = len(succeeded)
    n_fail = len(failed)
    n_total = n_succeed + n_fail
    rtn = {
        'statusCode':   status_code,
        'body':         f'Success on {n_succeed}, failed on {n_fail}; total {n_total}.'
    }

    return rtn
