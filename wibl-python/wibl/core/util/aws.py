from typing import Callable, IO, AnyStr, Optional
from functools import partial
import logging

from botocore.exceptions import ClientError


logger = logging.getLogger(__name__)


def generate_get_s3_object(boto_s3_client) -> Callable[[str, str], Optional[IO[AnyStr]]]:
    def open_s3_object(client, bucket: str, key: str) -> Optional[IO[AnyStr]]:
        try:
            return client.get_object(Bucket=bucket, Key=key)['Body']
        except ClientError as e:
            logger.warning(f"Unabled to open {key} in bucket {bucket}, reason: {e.response['Error']['Code']}.")
            return None

    return partial(open_s3_object, boto_s3_client)
