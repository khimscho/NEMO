import json

import boto3
from botocore.config import Config
from botocore.exceptions import ClientError

config = Config(
    region_name = 'us-east-2'
)
client = boto3.client('lambda', config=config)

fn_name = 'unhjhc-wibl-visualization'
payload = {
    "body": {
        "observer_name": "Test observer",
        "dest_key": "test_map_1",
        "source_keys": [
            "D57112C4-6F5C-4398-A920-B3D51A6AEAFB.json"
        ]
  }
}

try:
    response = client.invoke(
        FunctionName=fn_name,
        Payload=json.dumps(payload),
        LogType="Tail",
    )
    resp_body = response['Payload'].read()
    print(f"Response from calling {fn_name} was: {resp_body}")
except ClientError as e:
    print(f"Error invoking function {fn_name}: {str(e)}")
    raise e
