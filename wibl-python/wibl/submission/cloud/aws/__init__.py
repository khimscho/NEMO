from pathlib import Path

from wibl.core import config

DEFAULT_CONFIG_RESOURCE_NAME = 'defaults/submission/cloud/aws/configure.json'


def get_config_file() -> Path:
    return config.get_config_file(DEFAULT_CONFIG_RESOURCE_NAME)
