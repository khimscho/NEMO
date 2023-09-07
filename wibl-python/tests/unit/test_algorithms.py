import unittest
import logging
from pathlib import Path

import xmlrunner

import numpy as np

import wibl.core.config as conf
import wibl.core.timestamping as ts
from wibl.processing import algorithms
from wibl.processing.algorithms.common import AlgorithmPhase
from wibl_manager import ManagerInterface, MetadataType


logger = logging.getLogger(__name__)


class TestAlgorithms(unittest.TestCase):
    def setUp(self) -> None:
        self.fixtures_dir = Path(Path(__file__).parent.parent, 'fixtures')

    def test_algo_dedup(self):
        # Initialize
        config_file = Path(self.fixtures_dir, 'configure.local.json')
        local_file = str(Path(self.fixtures_dir, 'test-algo-dedup.wibl'))
        config = conf.read_config(config_file)
        self.assertIsNotNone(config)
        manager: ManagerInterface = ManagerInterface(MetadataType.WIBL_METADATA, local_file, config['verbose'])
        self.assertIsNotNone(manager)

        # Load data
        source_data = ts.time_interpolation(local_file, config['elapsed_time_quantum'], verbose=config['verbose'],
                                            fault_limit=config['fault_limit'])
        self.assertIsNotNone(source_data)

        # Check algorithm information
        self.assertEqual(1, len(source_data['algorithms']))
        alg = source_data['algorithms'][0]
        self.assertEqual('deduplicate', alg['name'])
        self.assertEqual('', alg['params'])

        # Verify number of depth values in test data
        self.assertEqual(277, len(source_data['depth']['z']))

        # Apply algorithms (i.e., just deduplication)
        for algorithm, alg_name, params in algorithms.iterate(source_data['algorithms'],
                                                              AlgorithmPhase.AFTER_TIME_INTERP,
                                                              manager, local_file):
            source_data = algorithm(source_data, params, config)

        # Verify number of depth values after deduplication
        self.assertEqual(234, len(source_data['depth']['z']))
