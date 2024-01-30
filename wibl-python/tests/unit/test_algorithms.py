import unittest
from pathlib import Path

from wibl import config_logger_service
from wibl.core import Lineage
import wibl.core.config as conf
import wibl.core.timestamping as ts
from wibl.core.algorithm import AlgorithmDescriptor, UnknownAlgorithm, AlgorithmPhase, runner


logger = config_logger_service()


class TestAlgorithms(unittest.TestCase):
    def setUp(self) -> None:
        self.fixtures_dir = Path(Path(__file__).parent.parent, 'data')

    def test_algo_dedup(self):
        # Initialize
        config_file = Path(self.fixtures_dir, 'configure.local.json')
        local_file = str(Path(self.fixtures_dir, 'test-algo-dedup.wibl'))
        config = conf.read_config(config_file)
        self.assertIsNotNone(config)
        lineage: Lineage = Lineage()

        # First load data without processing algorithms to include data from all packets
        source_data = ts.time_interpolation(local_file, lineage, config['elapsed_time_quantum'],
                                            verbose=config['verbose'], fault_limit=config['fault_limit'],
                                            process_algorithms=False)
        self.assertIsNotNone(source_data)

        # Verify number of depth values in test data
        self.assertEqual(277, len(source_data['depth']['z']))

        # Next load data with processing enabled
        source_data = ts.time_interpolation(local_file, lineage, config['elapsed_time_quantum'],
                                            verbose=config['verbose'], fault_limit=config['fault_limit'])

        # Verify number of depth values after deduplication
        self.assertEqual(234, len(source_data['depth']['z']))

        # Examine algorithm metadata
        self.assertEqual(1, len(source_data['algorithms']))
        alg: AlgorithmDescriptor = source_data['algorithms'][0]
        self.assertEqual('deduplicate', alg.name)
        self.assertEqual('', alg.params)

        # Examine lineage metadata
        self.assertEqual(1, len(lineage.lineage))
        l: dict = lineage.lineage[0]
        self.assertEqual('Algorithm', l['type'])
        self.assertEqual('deduplicate', l['name'])
        self.assertEqual('', l['parameters'])
        self.assertTrue(l['source'].startswith('WIBL-'))
        self.assertEqual('1.0.0', l['version'])
        self.assertEqual('Selected 234 non-duplicate depths from 277 in input.', l['comment'])

    def test_algo_unknown(self):
        # Initialize
        config_file = Path(self.fixtures_dir, 'configure.local.json')
        local_file = str(Path(self.fixtures_dir, 'test-algo-unknown.wibl'))
        config = conf.read_config(config_file)
        self.assertIsNotNone(config)

        # Try to load data (which will fail because of the unknown algorithm)
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
        lineage: Lineage = Lineage()

        # First load data without processing algorithms to include data from all packets
        source_data = ts.time_interpolation(local_file, lineage, config['elapsed_time_quantum'],
                                            verbose=config['verbose'], fault_limit=config['fault_limit'],
                                            process_algorithms=False)
        self.assertIsNotNone(source_data)
        num_data_unfiltered: int = len(source_data['depth']['z'])
        self.assertEqual(246, num_data_unfiltered)

        # Next load data with processing enabled
        source_data = ts.time_interpolation(local_file, lineage, config['elapsed_time_quantum'],
                                            verbose=config['verbose'], fault_limit=config['fault_limit'])
        self.assertIsNotNone(source_data)
        num_data_filtered: int = len(source_data['depth']['z'])
        self.assertEqual(218, num_data_filtered)
        # There should be less data when algorithm processing is enabled
        self.assertTrue(num_data_filtered < num_data_unfiltered)

        # Examine algorithm metadata
        self.assertEqual(1, len(source_data['algorithms']))
        alg: AlgorithmDescriptor = source_data['algorithms'][0]
        self.assertEqual('nodatareject', alg.name)
        self.assertEqual('', alg.params)

        # Examine lineage metadata
        self.assertEqual(1, len(lineage.lineage))
        l: dict = lineage.lineage[0]
        self.assertEqual('Algorithm', l['type'])
        self.assertEqual('nodatareject', l['name'])
        self.assertEqual('', l['parameters'])
        self.assertTrue(l['source'].startswith('WIBL-'))
        self.assertEqual('1.0.0', l['version'])
        self.assertEqual(('Filtered 97 packets of 975 total. '
                          'Filtered 29 of 181 GNSS packets. '
                          'Filtered 29 of 124 Depth packets. '
                          'Filtered 39 of 181 SystemTime packets.'),
                         l['comment'])
