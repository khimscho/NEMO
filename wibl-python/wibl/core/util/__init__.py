from typing import Callable, IO, List, Dict, Any, AnyStr, Optional
import logging
import json
from pathlib import Path


logger = logging.getLogger(__name__)


def open_fs_file_read(directory: str, filename: str) -> Optional[IO[AnyStr]]:
    file_to_merge = Path(directory, filename)
    if not file_to_merge.exists() or not file_to_merge.is_file():
        logger.warning(f"Unabled to open {str(file_to_merge)}: Path does not exist or is not a file.")
        return None
    return open(file_to_merge, 'r')


def merge_geojson(open_geojson: Callable[[str, str], Optional[IO[AnyStr]]],
                  location: str, filenames: List[str], out_geojson_fp, *,
                  fail_on_error: bool = False) -> None:
    """
    Merge multiple geojson files into one geojson file.
    :param geojson_path:
    :param out_geojson_path:
    :param files_glob:
    :param files_to_merge:
    :param fail_on_error:
    :return:
    """
    out_features: List[Dict[str, Any]] = []

    out_dict: Dict[str, Any] = {
        'type': 'FeatureCollection',
        'crs': {
            'type': 'name',
            'properties': {
                'name': 'urn:ogc:def:crs:OGC:1.3:CRS84'
            }
        },
        'features': out_features
    }

    # Read each file to merge
    for filename in filenames:
        file_like = open_geojson(location, filename)
        if file_like is None:
            mesg = f"Unable to merge {filename} at {location}: Could not open resource."
            if fail_on_error:
                logger.error(mesg)
                raise Exception(mesg)
            else:
                logger.warning(mesg)
                continue

        tmp_geojson: Dict = json.load(file_like)

        if 'features' not in tmp_geojson:
            mesg = f"Unable to merge features from {filename} in location {location}: No features found."
            if fail_on_error:
                logger.error(mesg)
                raise Exception(mesg)
            else:
                logger.warning(mesg)
                continue

        tmp_feat: List[Dict[str, Any]] = tmp_geojson['features']
        if not isinstance(tmp_feat, list):
            mesg = (f"Unable to merge features from {filename} in location {location}: "
                    "Expected 'features' to be an array, but it is not.")
            if fail_on_error:
                logger.error(mesg)
                raise Exception(mesg)
            else:
                logger.warning(mesg)
                continue

        out_features.extend(tmp_feat)

    # Write out merged features
    json.dump(out_dict, out_geojson_fp)
