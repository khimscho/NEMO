import os
import unittest
import logging
import tempfile
from pathlib import Path

import xmlrunner

logger = logging.getLogger(__name__)

from wibl.core import config
from wibl.processing.cloud.aws import get_config_file as get_config_file_processing
from wibl.processing.cloud.aws import DEFAULT_CONFIG_RESOURCE_NAME as RSRC_PROC
from wibl.submission.cloud.aws import get_config_file as get_config_file_submission
from wibl.submission.cloud.aws import DEFAULT_CONFIG_RESOURCE_NAME as RSRC_SUB


class TestConfig(unittest.TestCase):
    def setUp(self) -> None:
        self.files_to_delete = []

    def tearDown(self) -> None:
        for f in self.files_to_delete:
            os.unlink(f)

    def test_get_config_file_aws_no_env(self):
        # Processing lambda
        proc_default_config_name: str = config.get_config_resource_filename(RSRC_PROC)
        proc_config_file: Path = get_config_file_processing()
        self.assertIsInstance(proc_config_file, Path)
        self.assertEqual(proc_default_config_name, str(proc_config_file))
        
        # Submission lambda
        sub_default_config_name: str = config.get_config_resource_filename(RSRC_SUB)
        sub_config_file: Path = get_config_file_submission()
        self.assertIsInstance(sub_config_file, Path)
        self.assertEqual(sub_default_config_name, str(sub_config_file))

    def test_get_config_file_env(self):
        # Processing lambda
        proc_default_config_name: str = config.get_config_resource_filename(RSRC_PROC)
        # Create dummy config so that transitive call to config.get_config_path will succeed
        proc_dummy_config_fd = tempfile.NamedTemporaryFile(delete=False)
        dummy_config_file = proc_dummy_config_fd.name
        proc_dummy_config_fd.close()
        self.files_to_delete.append(dummy_config_file)
        os.environ[config.DEFAULT_CONFIG_FILE_ENV_KEY] = dummy_config_file
        # Get config for processing lambda
        proc_config: Path = get_config_file_processing()
        self.assertIsInstance(proc_config, Path)
        self.assertEqual(dummy_config_file, str(proc_config))
        
        # Submission lambda
        sub_default_config_name: str = config.get_config_resource_filename(RSRC_PROC)
        # Create dummy config so that transitive call to config.get_config_path will succeed
        sub_dummy_config_fd = tempfile.NamedTemporaryFile(delete=False)
        dummy_config_file = sub_dummy_config_fd.name
        sub_dummy_config_fd.close()
        self.files_to_delete.append(dummy_config_file)
        os.environ[config.DEFAULT_CONFIG_FILE_ENV_KEY] = dummy_config_file
        # Get config for submission lambda
        sub_config: Path = get_config_file_submission()
        self.assertIsInstance(sub_config, Path)
        self.assertEqual(dummy_config_file, str(sub_config))


if __name__ == '__main__':
    unittest.main(
        testRunner=xmlrunner.XMLTestRunner(output='test-reports'),
        failfast=False, buffer=False, catchbreak=False
    )
