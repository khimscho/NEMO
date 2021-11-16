# \file datasource.py
# \brief Provide types to encapsulate cloud-primitives like getting and sending files
#
# The WIBL data logger system uses cloud processing for the files being collected, for
# example for timestamping, duplicate depth removal, data triage, conversion to GeoJSON
# format, and so forth.  In an attempt to allow this to happen on different cloud providers
# this code defines a dataclass to provide metadata on each item being processed, an abstract
# base class to convert to this format from whatever the cloud provider gives the base
# code, and a class to encapsulate the cloud primitives of getting an object from storage,
# and putting one back in.
#   The development system for this code was AWS, so there's probably some bias in how the
# configuration, etc., is managed.  However, the idea is that other cloud providers could be
# accommodated by sub-classing DataSource to manage translating the packet of information that
# the cloud system provides to the serverless code into a DataItem list, and also sub-classing
# CloudController for the code to get a file from an object store (obtain() method) to local
# store for processing, and to write a translated and processed file to an object store
# (transmit() method).
#   Since it can be difficult to test code in the cloud, the AWS code here also has a "local" mode
# where the expectation is that compute will be done on data on a local computer.  The JSON
# configuration element { "local": "True" } will turn this on.  This is not a requirement of the
# base class, however, so it's up to other implementations whether to support this or not.  (It's
# strongly recommended, however.)
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

from dataclasses import dataclass
from abc import ABC
from urllib.parse import unquote_plus
from shutil import copyfile
import json

@dataclass
class DataItem:
    source:     str
    sourcekey:  str
    localname:  str
    dest:       str
    destkey:    str

class DataSource(ABC):
    def __init__(self):
        pass

    def nextSource(self) -> DataItem:
        pass

class AWSSource(DataSource):
    def __init__(self, event, config):
        self.items = []
        for record in event['Records']:
            source_bucket = record['s3']['bucket']['name']
            source_object = unquote_plus(record['s3']['object']['key'])
            local_file = f'/tmp/{source_object}'
            dest_bucket = config['staging_bucket']
            dest_object = source_object + '.json'
            self.items.append(DataItem(source_bucket, source_object, local_file, dest_bucket, dest_object))
    
    def nextSource(self) -> DataItem:
        if self.items:
            rc = self.items.pop()
        else:
            rc = None
        return rc

class CloudController(ABC):
    def __init__(self):
        pass
    
    def obtain(self, meta: DataItem) -> str:
        pass
        
    def transmit(self, destination: str, meta: DataItem, data: str) -> None:
        pass

class AWSController(CloudController):
    def __init__(self, config):
        if config['local'] == 'True':
            self.local_mode = True
        else:
            self.local_mode = False
        self.destination = config['staging_bucket']
        
    def obtain(self, meta: DataItem) -> str:
        if self.local_mode:
            print(f'Downloading from bucket {meta.source} object {meta.sourcekey} to local file {meta.localname}')
            copyfile(meta.sourcekey, meta.localname)
        else:
            s3.Bucket(meta.source).download_file(meta.sourcekey, meta.localname)
        return meta.localname
    
    def transmit(self, meta: DataItem, data: str) -> None:
        if self.local_mode:
            print(f'Transmitting to {meta.dest}, output object key {meta.destkey} with data {data}')
            with open(meta.destkey, 'w') as f:
                json.dump(json.loads(data.decode('utf-8')), f, indent=4)
        else:
            s3.Bucket(self.destination).put_object(Key=meta.destkey, Body=data)
