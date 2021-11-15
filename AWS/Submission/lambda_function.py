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

s3 = boto3.resource('s3')

def transmit_geojson(source_object: str, provider_id: str, provider_auth: str, local_file: str):
    print('Source object is: ' + source_object)
    
    headers = {
        'x-auth-token': provider_auth
    }
    dest_object = provider_id + '-' + source_object.replace(".json", "")
    print('Destination object uniqueID is: ' + dest_object)
    print('Authorisation token is: ' + provider_auth)
    files = {
        'file': (local_file, open(local_file, 'rb')),
        'metadataInput': (None, '{\n    "uniqueID": "' + dest_object + '"\n}')
    }
    
    response = requests.post('https://www.ngdc.noaa.gov/ingest-external/upload/csb/test/geojson/' + dest_object, headers=headers, files=files)
    
    print(response)
    
    try:
        rc = response.json()['success']
        if not rc:
            rc = None
    except json.decoder.JSONDecodeError:
        rc = None

    return rc

def lambda_handler(event, context):
    rtn = {
        'statusCode': 200,
        'body': 'Transmission complete.'
    }
    
    # We need to find the provider's authorisation ID and identifier so that we can
    # annotate the call to the DCDB upload point.  These are stored in a JSON file in
    # the same directory as the Lambda source, with a well-known name
    with open('credentials.json') as f:
        creds = json.load(f)
    
    for record in event['Records']:
        source_bucket = record['s3']['bucket']['name']
        source_object =  unquote_plus(record['s3']['object']['key'])
        local_file = '/tmp/{}'.format(source_object)
        
        s3.Bucket(source_bucket).download_file(source_object, local_file)
        
        rc = transmit_geojson(str(source_object), creds['provider_id'], creds['provider_auth'], local_file)
        if rc is None:
            # This indicates that something went wrong in determining the return code, so we indicate
            rtn = {
                'statusCode': 400,
                'body': 'Transmission failed: ' + source_object
            }
            print('Transmission failed for ' + source_object)
    
    return rtn
