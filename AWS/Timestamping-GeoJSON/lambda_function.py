## \file lambda_function.py
# \brief Drive the conversion and time-stamping of logger binary files with output in GeoJSON
#
# The low-cost data loggers generate data as binary outputs which are time referenced to the logger's
# internal reference time.  For submission into the DCDB database, we need it to be referenced to UTC,
# and we need to interpolate the positioning data to the times of the depth measurements recorded; we
# also need to convert to GeoJSON so that it can be pulled into their databases more easily.  This script
# orchestrates this process for a single file, and is expected to run as an AWS Lambda in response to
# the file being uploaded into an incoming S3 bucket.  The output goes into a "well known" S3 bucket
# that is used to stage data before it goes off to DCDB (via another Lambda).
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
import boto3
import datetime as dt
from urllib.parse import unquote_plus
import uuid

import Timestamping as ts
import GeoJSONConvert as gj

s3 = boto3.resource('s3')

def lambda_handler(event, context):
    for record in event['Records']:
        source_bucket = record['s3']['bucket']['name']
        source_object = unquote_plus(record['s3']['object']['key'])
        local_file = '/tmp/{}'.format(source_object)
        
        print('Downloading from ' + source_bucket + ', object ' + source_object + ', to local file ' + local_file + '\n')
        
        s3.Bucket(source_bucket).download_file(source_object, local_file)
                
        source_data = ts.time_interpolation(local_file)
        source_data.outputfilename = "UNHJHC-" + source_object
        
        submit_data = gj.translate(source_data)
        
        # The GeoJSON gets written out to a separate bucket so that it can be sent to DCDB
        dest_bucket = 'csb-upload-dcdb-staging'
        dest_object = source_object + ".json"
        
        encoded_data = json.dumps(submit_data).encode("utf-8")
        
        print('Uploading to bucket ' + dest_bucket + ', object ' + dest_object + '\n')
        
        s3.Bucket(dest_bucket).put_object(Key=dest_object, Body=encoded_data)
    
    return {
        'statusCode': 200,
        'body': json.dumps('Processing completed.')
    }
