import pygmt
from osgeo import gdal
import rasterio

gdal.UseExceptions()

RASTER_NODATA = -99999.
RASTER_RES = 0.0005

gebco_path: str = 'data/gebco_2023/GEBCO_2023.nc'
buffer = 0.5
buffer_inset = 4 * buffer
sounding_path: str = 'data/bigfile-md.json'
sounding_rast: str = 'data/bigfile-md.tif'

# Generate GeoTIFF of soundings from GeoJSON file
try:
    gdal.Rasterize(sounding_rast, sounding_path, format='GTiff',
                   outputSRS='EPSG:4326', xRes=RASTER_RES, yRes=-RASTER_RES,
                   noData=RASTER_NODATA, creationOptions=['COMPRESS=DEFLATE', 'ZLEVEL=9'],
                   attribute='depth')
except Exception as e:
    # TODO: Handle this exception once we move this to library code
    pass

# Get bounds from raster metadata
with rasterio.open(sounding_rast) as d:
    xmin = d.bounds.left
    xmax = d.bounds.right
    ymin = d.bounds.bottom
    ymax = d.bounds.top

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

pygmt.config(FONT_TITLE='20p,Helvetica')
f = pygmt.Figure()

title = f"Soundings from '{sounding_path}'"
f.basemap(region=region,
          projection='M16c',
          frame=["afg", f"+t{title}"])

f.grdimage(gebco_path,
           region=region,
           projection="M16c",
           cmap='terra')

f.colorbar(position="JBC", frame=["x+lGEBCO 2023 Bathymetry", "y+lm"])

# Plot soundings
f.grdview(sounding_rast, cmap=True)

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

# f.savefig('pygmt_test_gtiff.png')
f.psconvert(prefix='pygmt_test_gtiff', fmt='f', resize='+m0.2c')
