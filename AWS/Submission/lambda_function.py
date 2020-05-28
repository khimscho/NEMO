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
    'x-auth-token': 'eyJpc3MiOiJOQ0VJIiwic3ViIjoiVEVTVEVSIiwiYXVkIjoiaW5nZXN0LWV4dGVybmFsIiwicm9sZXMiOiJST0xFX1RFU1RfQ09OVFJJQlVUT1IifQ==.1DUCdyoKbscbRb+3D/zDyFDNdtAlIyhRL0PRKmMEeCg='
    }
    
    #with open(upload_path, 'r') as myfile:
    #    data = myfile.read()
    #    print(data)
        
    files = {
    'file': (upload_path, open(upload_path, 'rb')),
    'metadataInput': (None, '{\n    "uniqueID": "UNHJHC-d23a3c32-6fa1-11ea-bc55-0242ac130003"\n}'),
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
