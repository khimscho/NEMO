import json
import tempfile
from pathlib import Path
from typing import Dict, List, Any
import logging

import pygmt
from osgeo import gdal
import rasterio

gdal.UseExceptions()

RASTER_NODATA = -99999.
RASTER_MAX = 40_000
RASTER_RES = 0.0009
REGION_INSET_MULT = 4

gebco_path: Path = Path('data/gebco_2023/GEBCO_2023.nc')
geojson_path: Path = Path('data/GeoJSON')
observer_name: str = '$VESSEL_NAME'


logger = logging.getLogger(__name__)


def merge_geojson(geojson_path: Path, out_geojson_fp, *,
                  files_glob: str = '*.json',
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
    for file_to_merge in geojson_path.glob(files_glob):
        if not file_to_merge.exists() or not file_to_merge.is_file():
            mesg = f"Unabled to merge {str(file_to_merge)}: Path does not exist or is not a file."
            if fail_on_error:
                logger.error(mesg)
                raise Exception(mesg)
            else:
                logger.warning(mesg)
                continue
        with open(file_to_merge) as tmp_f:
            tmp_geojson: Dict = json.load(tmp_f)

        if 'features' not in tmp_geojson:
            mesg = f"Unable to merge features from {str(file_to_merge)}: No features found."
            if fail_on_error:
                logger.error(mesg)
                raise Exception(mesg)
            else:
                logger.warning(mesg)
                continue

        tmp_feat: List[Dict[str, Any]] = tmp_geojson['features']
        if not isinstance(tmp_feat, list):
            mesg = (f"Unable to merge features from {str(file_to_merge)}: "
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


# Merge GeoJSON files into a single file
# Create merged GeoJSON file in a temporary directory and get its name
merged_geojson_fp = tempfile.NamedTemporaryFile(mode='w',
                                                encoding='utf-8',
                                                newline='\n',
                                                suffix='.json',
                                                delete=False)
sounding_path: Path = Path(merged_geojson_fp.name)
sounding_rast: Path = sounding_path.with_suffix('.tif')
# Now merge and then close the merged file so that we can rasterize it next.
merge_geojson(geojson_path, merged_geojson_fp)
merged_geojson_fp.close()

# Generate GeoTIFF of soundings from GeoJSON file
try:
    gdal.Rasterize(sounding_rast, sounding_path, format='GTiff',
                   outputSRS='EPSG:4326', xRes=RASTER_RES, yRes=-RASTER_RES,
                   noData=RASTER_NODATA, creationOptions=['COMPRESS=DEFLATE', 'ZLEVEL=9'],
                   attribute='depth', where=f"depth > 0 AND depth < {RASTER_MAX}")
    logger.debug(f"Deleting merged soundings file {str(sounding_path)}")
    sounding_path.unlink()
except Exception as e:
    # TODO: Handle this exception once we move this to library code
    print(str(e))
    raise e

# Get bounds from raster metadata
with rasterio.open(sounding_rast) as d:
    xmin = d.bounds.left
    xmax = d.bounds.right
    ymin = d.bounds.bottom
    ymax = d.bounds.top

# Buffer map bounds around data based on the maximum of the x- and y-ranges
#   to avoid extremely tall or wide maps
xrng = xmax - xmin
yrng = ymax - ymin
buffer = max(xrng, yrng) / 2
buffer_inset = REGION_INSET_MULT * buffer

# Setup region, projections and polygons for main map an insets...
region = [xmin - buffer,
          xmax + buffer,
          ymin - buffer,
          ymax + buffer]
region_poly = [[region[0], region[2], region[1], region[3]]]

region_inset = [xmin - buffer_inset,
                xmax + buffer_inset,
                ymin - buffer_inset,
                ymax + buffer_inset]

global_poly = [[region_inset[0],
                region_inset[2],
                region_inset[1],
                region_inset[3]]]

center_lon: float = (xmin + xmax) / 2
center_lat: float = (ymin + ymax) / 2
global_inset_proj = f"G{center_lon}/{center_lat}/?"

pygmt.config(FONT_TITLE='20p,Helvetica',
             FONT_LABEL='12p,Helvetica')
f = pygmt.Figure()

title = f"Soundings from '{observer_name}'"
f.basemap(region=region,
          projection='M16c',
          frame=["afg", f"+t{title}"])

f.grdimage(gebco_path,
           region=region,
           projection="M16c",
           cmap='terra')

f.colorbar(position="JBC", frame=["x+lGEBCO 2023 Bathymetry", "y+lm"])

# Plot soundings
f.grdimage(sounding_rast, cmap='drywet', nan_transparent=True)
f.colorbar(position="JLM+o-2.0c/0c+w4c",
           box="+gwhite@30+p0.8p,black",
           frame=["x+lSounding depth", "y+lm"])

# Plot global inset
with f.inset(
    position="jTR+w2c+o0.25c/0.25c",
    box="+gwhite+p1p+c0.1c"
):
    f.coast(
        region='g',
        projection=global_inset_proj,
        land='grey',
        water='white',
        frame='gf'
    )
    f.plot(data=global_poly, style="r+s", pen="0.5p,blue")

# Plot region inset
with f.inset(
    position="jTR+w2c/2.2c+o0.25c/2.55c",
    box="+gwhite+p1p+c0.1c"
):
    f.coast(
        region=region_inset,
        projection='M2c',
        land='grey',
        water='white'
    )
    f.plot(data=region_poly, style="r+s", pen="0.5p,blue")

f.psconvert(prefix=sounding_path.stem, fmt='f', resize='+m0.2c')
