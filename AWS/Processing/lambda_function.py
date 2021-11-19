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
from typing import Dict, Any

# Local modules
import config as conf
import datasource as ds
import Timestamping as ts
import GeoJSONConvert as gj
from algorithms.deduplicate import deduplicate as dedup

s3 = boto3.resource('s3')
    
def read_local_event(event_file: str) -> Dict:
    """For local testing, you need to simulate an AWS Lambda event stucture to give the code something
       to do.  You can download the JSON for a typical event from the AWS Lambda console and store it
       locally.  This helper function can be called interactively to load this information.
    """
    with open(event_file) as f:
        event = json.load(f)
    return event

def process_item(item: ds.DataItem, controller: ds.CloudController, config: Dict[str,Any]) -> bool:
    """Implement the business logic to translate the file from WIBL binary into a GeoJSON file.  The
       file with metadata in 'item' is pulled from object store using 'controller.obtain()', run through
       the translation and time-stamping from the Timestamping module, and then converted to GeoJSON
       format.  The 'controller.transmit()' method is then used to send the file into the destination
       object store so that the file is effectively cached until it is uploaded to the end-point database.
       Note that this code does not clean up the input object, so all files being uploaded to the cloud
       might potentially stay in the input object store unless otherwise deleted.
    """
    local_file = controller.obtain(item)
    try:
        source_data = ts.time_interpolation(local_file, config['elapsed_time_quantum'], config['verbose'])
    except ts.NoTimeSource:
        print('Failed to convert data: no time source known.')
        return False
    except ts.NewerDataFile:
        print('Failed to convert data: file data format is newer than latest version known to code.')
        return False
    except ts.NoData:
        print('Failed to convert data: no bathymetric data in file.')
        return False
    
    # We now have the option to run any sub-algorithms on the data before converting to GeoJSON
    # and encoding for output to the staging area for upload.
    for alg in source_data['algorithms']:
        algname = alg['name']
        if algname == 'deduplicate':
            if config['verbose']:
                print(f'Applying algorithm {algname}')
            source_data = dedup.deduplicate_depth(source_data, alg['params'], config)
        else:
            print(f'Warning: unknown algorithm {algname}')
    
    submit_data = gj.translate(source_data, config)
    encoded_data = json.dumps(submit_data).encode('utf-8')
    controller.transmit(item, encoded_data)
    return True
    
    
def lambda_handler(event, context):
    try:
        # The configuration file for the algorithm should be in the same directory as the lambda function file,
        # and has a "well known" name.  We could attempt to something smarter here, but this is probably enough
        # for now.
        config = conf.read_config('configure.json')
    except conf.BadConfiguration:
        return {
            'statusCode': 400,
            'body': 'Bad configuration'
        }
    
    # We instantiate the AWS versions of DataSource and CloudController explicitly, since we can't be using anything
    # else given the rest of the infrastructure here, which is AWS specific.
    source = ds.AWSSource(event, config)
    controller = ds.AWSController(config)
    
    p = source.nextSource()
    while p is not None:
        if not process_item(p, controller, config):
            print(f'Abandoning processing of {p.sourcekey} due to errors.')
        p = source.nextSource()

    return {
        'statusCode': 200,
        'body': json.dumps('Processing completed.')
    }
