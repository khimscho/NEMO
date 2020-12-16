# Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
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
import boto3
from urllib.parse import unquote_plus
import uuid
import os
import sys
import requests



s3_client = boto3.client('s3')

def process_geojson(upload_path, key):
    
    headers = {
    'x-auth-token': 'YOUR-AUTH-TOKEN-HERE'
    }
    
    #with open(upload_path, 'r') as myfile:
    #    data = myfile.read()
    #    print(data)
        
    files = {
    'file': (upload_path, open(upload_path, 'rb')),
    'metadataInput': (None, '{\n    "uniqueID": "YOUR-UNIQUE-ID-HERE"\n}'),
    }
    
    response = requests.post('https://www.ngdc.noaa.gov/ingest-external/upload/csb/test/geojson/MYORG-f8c469f8-df38-11e5-b86d-9a79f06e9478', headers=headers, files=files)
    
    print(response.text)

def lambda_handler(event, context):
    # TODO implement
    
    for record in event['Records']:
        bucket = record['s3']['bucket']['name']
        key =  unquote_plus(record['s3']['object']['key'])
        download_path = '/tmp/{}{}'.format(uuid.uuid4(), key)
        
        s3_client.download_file(bucket, key, download_path)
        
        process_geojson(download_path, str(key))
        
    return {
        'statusCode': 200,
        'body': json.dumps('Hello from Lambda!')
    }
