from pathlib import Path

import pytest

# Note: need to import localstack_url fixture since other fixtures depend on it.
from ..fixtures import localstack_url, data_path, s3_local

GEOJSON_BUCKET_NAME: str = 'geojson-test-bucket'


@pytest.fixture(scope="module")
def populated_data(data_path, s3_local):
    """
    Fixture used to amortize the cost of inserting soundings in to the database for the
    purposes of testing queries.
    :param data_path:
    :param dynamodb_local: Fixture representing boto3 DynamoDB resource
    :return: boto3 DynamoDB client
    """
    bucket = s3_local.Bucket(GEOJSON_BUCKET_NAME)
    bucket.create()

    geojson_path: Path = data_path / 'geojson'
    for geojson_file in geojson_path.glob('*.json'):
        bucket.upload_file(geojson_file, geojson_file.name)

    yield bucket


def test_data_loaded(populated_data, data_path, s3_local):
    waiter = s3_local.meta.client.get_waiter('object_exists')
    geojson_path: Path = data_path / 'geojson'
    for geojson_file in geojson_path.glob('*.json'):
        waiter.wait(Bucket=GEOJSON_BUCKET_NAME, Key=geojson_file.name)
        response = s3_local.meta.client.head_object(Bucket=GEOJSON_BUCKET_NAME, Key=geojson_file.name)
        assert geojson_file.stat().st_size == response['ContentLength']
