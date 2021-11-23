##\file upload_wibl_file.py
# \brief Send a WIBL binary file into AWS S3 for processing
#
# This is a simple command-line utility to send a given WIBL binary file to
# the configured AWS S3 bucket that triggers the Lambda functions for processing.
# Note that this does not deal with credentials: you have to have the appropriate
# credentials set up in your local environment to allow the upload.
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

import boto3
import uuid
import argparse as arg
from sys import exit

def main():
    parser = arg.ArgumentParser(description = 'Upload WIBL logger files (in a limited capacity)')
    parser.add_argument('-b', '--bucket', type=str, help = 'Set the upload bucket name (string)')
    parser.add_argument('-j', '--json', action='store_true', help = 'Add .json extension to UUID for upload key')
    parser.add_argument('input', type=str, help = 'WIBL format input file')

    optargs = parser.parse_args()

    if not optargs.input:
        print('Error: must have an input file!')
        exit(1)
    else:
        filename = optargs.input
  
    if optargs.bucket:
        bucket = optargs.bucket
    else:
        bucket = 'csb-upload-ingest-bucket'
    
    if optargs.json:
        file_extension = '.json'
    else:
        file_extension = ''

    s3 = boto3.resource('s3')
    dest_bucket = s3.Bucket(bucket)
    try:
        obj_key = str(uuid.uuid4()) + file_extension
        dest_bucket.upload_file(Filename=filename, Key=obj_key)
        print(f'Successfully uploaded {filename} to bucket {bucket} for object {obj_key}.')
    except:
        print(f"Failed to upload file to CSB ingest bucket")

if __name__ == "__main__":
    main()
