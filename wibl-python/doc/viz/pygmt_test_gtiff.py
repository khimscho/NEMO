import pygmt
import xarray
import rioxarray

gebco_path: str = 'data/gebco_2023/GEBCO_2023.nc'
buffer = 0.1
buffer_index = 10.0

soundings = rioxarray.open_rasterio('data/bigfile-md.tif')
xmin = float(min(soundings.x).data)
xmax = float(max(soundings.x).data)
ymin = float(min(soundings.y).data)
ymax = float(max(soundings.y).data)
region = [xmin - buffer,
          xmax + buffer,
          ymin - buffer,
          ymax + buffer]
print(region)

region_inset = [xmin - buffer_index,
                xmax + buffer_index,
                ymin - buffer_index,
                ymax + buffer_index]
print(region_inset)

gebco_ds: xarray.Dataset = xarray.open_dataset(gebco_path)
gebco_el: xarray.DataArray = gebco_ds['elevation']

f = pygmt.Figure()
f.grdimage(gebco_el,
           region=region,
           projection="M16c",
           frame="afg",
           cmap=True)

f.coast(
    region=region,
    shorelines=True,
    land="lightgreen",
    #water="lightblue",
    projection="M16c",
)

# f.plot(x=example_df.x, y=example_df.y,
#        style="c0.3c", fill="white", pen="black",
#        label=f"Soundings from '{soundings_filename}'")

# f.grdimage(soundings)

f.colorbar(position="JBC", frame=["x+lGEBCO 2023 depth", "y+lm"])

# f.legend()

with f.inset(
    position="jBR+o0.1c",
    box="+gwhite+p1p",
    region=region_inset,
    projection="M2c",
):
    # Highlight the Japan area in "lightbrown"
    # and draw its outline with a pen of "0.2p".
    f.coast(
        land="grey",
        water="white"
    )
    # Plot a rectangle ("r") in the inset map to show the area of the main
    # figure. "+s" means that the first two columns are the longitude and
    # latitude of the bottom left corner of the rectangle, and the last two
    # columns the longitude and latitude of the uppper right corner.
    rectangle = [[region[0], region[2],
                  region[1], region[3]]]
    f.plot(data=rectangle, style="r+s", pen="2p,blue")

f.savefig('pygmt_test_gtiff.png')
