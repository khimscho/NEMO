import unittest
import logging
from pathlib import Path

from wibl.core import Lineage
import wibl.core.config as conf
import wibl.core.timestamping as ts
from wibl.core.algorithm import AlgorithmDescriptor, UnknownAlgorithm, AlgorithmPhase, runner


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

        # Load data
        lineage: Lineage = Lineage()
        source_data = ts.time_interpolation(local_file, lineage, config['elapsed_time_quantum'],
                                            verbose=config['verbose'], fault_limit=config['fault_limit'])
        self.assertIsNotNone(source_data)

        # Check algorithm information
        self.assertEqual(1, len(source_data['algorithms']))
        alg: AlgorithmDescriptor = source_data['algorithms'][0]
        self.assertEqual('deduplicate', alg.name)
        self.assertEqual('', alg.params)

        # TODO: Check lineage for deduplication

        # Verify number of depth values in test data
        self.assertEqual(277, len(source_data['depth']['z']))

        # Apply algorithms (i.e., just deduplication)
        for algorithm, alg_name, params in runner.iterate(source_data['algorithms'],
                                                          AlgorithmPhase.AFTER_TIME_INTERP,
                                                          local_file):
            source_data = algorithm(source_data, params, lineage, config['verbose'])

        # Verify number of depth values after deduplication
        self.assertEqual(234, len(source_data['depth']['z']))

    def test_algo_unknown(self):
        # Initialize
        config_file = Path(self.fixtures_dir, 'configure.local.json')
        local_file = str(Path(self.fixtures_dir, 'test-algo-unknown.wibl'))
        config = conf.read_config(config_file)
        self.assertIsNotNone(config)

        # Try to load data (which will fail because of the unknown algorithm
        lineage: Lineage = Lineage()
        exception_raised: bool = False
        try:
            source_data = ts.time_interpolation(local_file, lineage, config['elapsed_time_quantum'],
                                                verbose=config['verbose'], fault_limit=config['fault_limit'])
        except UnknownAlgorithm as e:
            exception_raised = True
            self.assertEqual(f"Unknown algorithm 'magicalgorithm' for {local_file}.", str(e))
        self.assertTrue(exception_raised)

    def test_algo_nodatareject(self):
        # Initialize
        config_file = Path(self.fixtures_dir, 'configure.local.json')
        local_file = str(Path(self.fixtures_dir, 'test-bin-algo-nodatafilter.wibl'))
        config = conf.read_config(config_file)
        self.assertIsNotNone(config)

        # Load data
        lineage: Lineage = Lineage()
        source_data = ts.time_interpolation(local_file, lineage, config['elapsed_time_quantum'],
                                            verbose=config['verbose'], fault_limit=config['fault_limit'])
        self.assertIsNotNone(source_data)
