import os
from pathlib import Path
from typing import List, Dict, Any
import tempfile
import json

import pytest

from wibl import config_logger_service
from wibl.visualization.soundings import map_soundings
from wibl.core.util import merge_geojson
from wibl.core.util.aws import generate_get_s3_object, open_s3_hdf5_as_xarray

from tests.fixtures import data_path
# Note: need to import localstack_url fixture since other fixtures depend on it.
from tests.aws.fixtures import localstack_url, s3_local_rsrc

GEOJSON_BUCKET_NAME: str = 'geojson-test-bucket'


logger = config_logger_service()


@pytest.fixture(scope="module")
def populated_geojson_data(data_path, s3_local_rsrc):
    """
    Fixture used to amortize the cost of inserting soundings in to the database for the
    purposes of testing queries.
    :param data_path:
    :param dynamodb_local: Fixture representing boto3 DynamoDB resource
    :return: boto3 DynamoDB client
    """
    bucket = s3_local_rsrc.Bucket(GEOJSON_BUCKET_NAME)
    bucket.create()

    geojson_path: Path = data_path / 'geojson'
    for geojson_file in geojson_path.glob('*.json'):
        bucket.upload_file(geojson_file, geojson_file.name)

    yield bucket


@pytest.fixture(scope="module")
def merged_geojson_fp():
    merged_geojson_fp = tempfile.NamedTemporaryFile(mode='w',
                                                    encoding='utf-8',
                                                    newline='\n',
                                                    suffix='.json',
                                                    delete=False)
    yield merged_geojson_fp
    os.unlink(merged_geojson_fp.name)


def test_data_loaded(populated_geojson_data, data_path, s3_local_rsrc):
    waiter = s3_local_rsrc.meta.client.get_waiter('object_exists')
    geojson_path: Path = data_path / 'geojson'
    for geojson_file in geojson_path.glob('*.json'):
        waiter.wait(Bucket=GEOJSON_BUCKET_NAME, Key=geojson_file.name)
        response = s3_local_rsrc.meta.client.head_object(Bucket=GEOJSON_BUCKET_NAME, Key=geojson_file.name)
        assert geojson_file.stat().st_size == response['ContentLength']


def test_merge_geojson_s3(data_path, s3_local_rsrc, populated_geojson_data, merged_geojson_fp):
    geojson_path: Path = data_path / 'geojson'
    files_to_merge: List[str] = [f.name for f in geojson_path.glob('*.json')]

    merged_geojson_path: Path = Path(merged_geojson_fp.name)
    merge_geojson(generate_get_s3_object(s3_local_rsrc.meta.client),
                  GEOJSON_BUCKET_NAME, files_to_merge, merged_geojson_fp)
    merged_geojson_fp.close()

    assert merged_geojson_path.exists()
    assert merged_geojson_path.is_file()
    merged_dict: Dict[str, Any] = json.load(merged_geojson_path.open('r'))
    assert len(merged_dict['features']) == 18013


def test_map_soundings(data_path, s3_local_rsrc, populated_geojson_data, merged_geojson_fp):
    """
    :param data_path:
    :param s3_local_rsrc:
    :param populated_data:
    :param merged_geojson_fp:
    :return:
    """
    geojson_path: Path = data_path / 'geojson'
    files_to_merge: List[str] = [f.name for f in geojson_path.glob('*.json')]

    merged_geojson_path: Path = Path(merged_geojson_fp.name)
    merge_geojson(generate_get_s3_object(s3_local_rsrc.meta.client),
                  GEOJSON_BUCKET_NAME, files_to_merge, merged_geojson_fp)
    merged_geojson_fp.close()

    map_filename: Path = map_soundings(merged_geojson_path,
                                       '$VESSEL_NAME',
                                       'VESSEL_NAME')

    assert map_filename.exists() and map_filename.is_file()
    assert map_filename.stat().st_size > 0
