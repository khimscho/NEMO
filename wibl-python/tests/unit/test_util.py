import json
import tempfile
from pathlib import Path
from typing import List, Dict, Any

from wibl import config_logger_service
from wibl.core.util import merge_geojson, open_fs_file_read

from tests.fixtures import data_path


logger = config_logger_service()


def test_merge_geojson(data_path):
    geojson_path = data_path / 'geojson'
    files_to_merge: List[str] = [f.name for f in geojson_path.glob('*.json')]

    merged_geojson_fp = tempfile.NamedTemporaryFile(mode='w',
                                                    encoding='utf-8',
                                                    newline='\n',
                                                    suffix='.json',
                                                    delete=False)
    merged_geojson_path: Path = Path(merged_geojson_fp.name)
    merge_geojson(open_fs_file_read,
                  str(geojson_path), files_to_merge, merged_geojson_fp)
    merged_geojson_fp.close()

    assert merged_geojson_path.exists()
    assert merged_geojson_path.is_file()
    merged_dict: Dict[str, Any] = json.load(merged_geojson_path.open('r'))
    assert len(merged_dict['features']) == 18013
