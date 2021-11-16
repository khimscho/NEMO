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

def read_local_event(event_file: str) -> Dict:
    """For local testing, you need to simulate an AWS Lambda event stucture to give the code something
       to do.  You can download the JSON for a typical event from the AWS Lambda console and store it
       locally.  This helper function can be called interactively to load this information.
    """
    with open(event_file) as f:
        event = json.load(f)
    return event

def transmit_geojson(source_object: str, provider_id: str, provider_auth: str, local_file: str, config: Dict[str,Any]) -> bool:
    if config['verbose']:
        print('Source object is: ' + source_object)
    
    headers = {
        'x-auth-token': provider_auth
    }
    dest_object = provider_id + '-' + source_object.replace(".json", "")
    if config['verbose']:
        print('Destination object uniqueID is: ' + dest_object)
        print('Authorisation token is: ' + provider_auth)

    files = {
        'file': (local_file, open(local_file, 'rb')),
        'metadataInput': (None, '{\n    "uniqueID": "' + dest_object + '"\n}')
    }
    
    if config['local']:
        upload_point = config['upload_point']
        print(f'Transmitting to {upload_point} for source object {source_object} to destination {dest_object}.')
        rc = True
    else:
        response = requests.post(config['upload_point'] + dest_object, headers=headers, files=files)
        print(response)
        try:
            rc = response.json()['success']
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
        local_file = controller.obtain(item)
        rc = transmit_geojson(item.sourcekey, creds['provider_id'], creds['provider_auth'], local_file, config)
        if rc is None:
            failed.append(item)
        else:
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
