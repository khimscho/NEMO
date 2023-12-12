from pathlib import Path

import pygmt
from osgeo import gdal
import rasterio

gdal.UseExceptions()

RASTER_NODATA = -99999.
RASTER_MAX = 40_000
RASTER_RES = 0.0009
REGION_INSET_MULT = 4

gebco_path: str = 'data/gebco_2023/GEBCO_2023.nc'
# sounding_str: str = 'data/bigfile-md.json'
sounding_str: str = 'data/pnw-b12_2.0.json'
sounding_path: Path = Path(sounding_str)
sounding_rast: Path = sounding_path.with_suffix('.tif')

# TODO: Merge GeoJSON into a single file
# $ ogrmerge.py -o ~/Downloads/GeoJSON/merged.json ~/Downloads/GeoJSON/*.json -single


# Generate GeoTIFF of soundings from GeoJSON file
try:
    gdal.Rasterize(sounding_rast, sounding_path, format='GTiff',
                   outputSRS='EPSG:4326', xRes=RASTER_RES, yRes=-RASTER_RES,
                   noData=RASTER_NODATA, creationOptions=['COMPRESS=DEFLATE', 'ZLEVEL=9'],
                   attribute='depth', where=f"depth < {RASTER_MAX}")
except Exception as e:
    # TODO: Handle this exception once we move this to library code
    pass

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

title = f"Soundings from '{sounding_str}'"
f.basemap(region=region,
          projection='M16c',
          frame=["afg", f"+t{title}"])

f.grdimage(gebco_path,
           region=region,
           projection="M16c",
           cmap='terra')

f.colorbar(position="JBC", frame=["x+lGEBCO 2023 Bathymetry", "y+lm"])

# Plot soundings
f.grdimage(sounding_rast, cmap='haxby', nan_transparent=True)
f.colorbar(position="JRM+o-2.0c/0c+w4c",
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
