from pathlib import Path
import tempfile
import json
from typing import List

import boto3

from wibl import config_logger_service
from wibl.core import getenv
import wibl.core.config as conf
import wibl.core.datasource as ds
# import wibl.core.notification as nt
from wibl.visualization.cloud.aws import get_config_file
from wibl.core.util import merge_geojson
from wibl.core.util.aws import generate_get_s3_object
from wibl.visualization.soundings import map_soundings


logger = config_logger_service()
s3 = boto3.resource('s3')


def lambda_handler(event, context):
    try:
        # The configuration file for the algorithm should be in the same directory as the lambda function file,
        # and has a "well known" name.  We could attempt to something smarter here, but this is probably enough
        # for now.
        config = conf.read_config(get_config_file())
    except conf.BadConfiguration:
        return {
            'statusCode': 400,
            'body': 'Bad configuration'
        }

    if config['verbose']:
        logger.info(f"event: {event}")
        # logger.info(f"notifier: {getenv('NOTIFICATION_ARN')}")

    # source: ds.MultiDataSource = ds.AWSMultiSource(event, config)
    controller = ds.AWSController(config)
    # notifier = nt.SNSNotifier(getenv('NOTIFICATION_ARN'))

    # Since we're getting SNS notifications, there can only be one item to process
    # for each invocation.
    # item: ds.MultiDataItem = source.nextSource()
    # # Validate item
    # if item.meta is None or 'observer_name' not in item.meta:
    #     return {
    #         'statusCode': 400,
    #         'body': f"Required property 'observer_name' not defined in event metadata."
    #     }

    source_store: str = getenv('STAGING_BUCKET')

    if 'body' not in event:
        return {
            'statusCode': 400,
            'body': 'Body not found in event.'
        }
    body: dict = json.loads(event['body'])

    if 'observer_name' not in body:
        return {
            'statusCode': 400,
            'body': f"Property 'observer_name' not found in event body: {body}"
        }
    observer_name: str = body['observer_name']

    if 'dest_key' not in body:
        return {
            'statusCode': 400,
            'body': f"Property 'dest_key' not found in event body: {body}"
        }
    dest_key: str = body['dest_key']

    if 'source_keys' not in body:
        return {
            'statusCode': 400,
            'body': f"Property 'source_keys' not found in event body: {body}"
        }
    source_keys: List[str] = body['source_keys']

    # First merge GeoJSON soundings from S3 into a single local GeoJSON file
    merged_geojson_fp = tempfile.NamedTemporaryFile(mode='w',
                                                    encoding='utf-8',
                                                    newline='\n',
                                                    suffix='.json',
                                                    delete=False)
    merged_geojson_path: Path = Path(merged_geojson_fp.name)
    try:
        merge_geojson(generate_get_s3_object(s3.meta.client),
                      source_store, source_keys, merged_geojson_fp,
                      fail_on_error=True)
    except Exception as e:
        merged_geojson_fp.close()
        merged_geojson_path.unlink()
        return {
            'statusCode': 500,
            'body': str(e)
        }
    merged_geojson_fp.close()

    # Map soundings into local temporary file
    # map_filename: Path = map_soundings(merged_geojson_path,
    #                                    item.meta['observer_name'],
    #                                    item.dest_key)
    map_filename: Path = map_soundings(merged_geojson_path,
                                       observer_name,
                                       dest_key)

    # Upload map to S3 destination bucket
    controller.upload(str(map_filename), map_filename.name)

    # Send notification that map was created
    # notifier.notify(ds.DataItem(source_store=item.source_store,
    #                             source_key='',
    #                             source_size=None,
    #                             localname=None,
    #                             dest_store=controller.destination,
    #                             dest_key=map_filename.name,
    #                             dest_size=map_filename.stat().st_size))

    return {
        'statusCode': 200,
        'body': f"Successfully uploaded map named {map_filename.name}."
    }
