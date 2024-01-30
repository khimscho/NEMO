from pathlib import Path

import pygmt
import xarray

import geopandas


out_plot: Path = Path('pygmt_test.pdf')
if out_plot.exists():
    out_plot.unlink()


# Read point data and generate region
buffer = 0.1
example_data = 'data/b12_v3_1_0_example.json'
example_df = geopandas.read_file(example_data)
example_df['x'] = example_df.geometry.apply(lambda p: p.x)
example_df['y'] = example_df.geometry.apply(lambda p: p.y)
region = [example_df.total_bounds[0] - buffer,
          example_df.total_bounds[2] + buffer,
          example_df.total_bounds[1] - buffer,
          example_df.total_bounds[3] + buffer]

# Plot GEBCO basemap
gebco_path: str = '/tmp/gmtout/GEBCO_2023.nc'
gebco_ds: xarray.Dataset = xarray.open_dataset(gebco_path)
gebco_el: xarray.DataArray = gebco_ds['elevation']
# gebco_el = pygmt.load_dataarray(f"{gebco_path}?elevation")
# region = [-71.0, -70.0, 42.5, 43.5]

f = pygmt.Figure()
f.grdimage(gebco_el,
           region=region,
           projection="M16c",
           frame="afg",
           cmap=True)

f.colorbar(position="JML+o1.4c/0c+w7c/0.5c", frame=["xa1000f500+lElevation", "y+lm"])

# Plot coast
f.coast(
    region=region,
    shorelines=True,
    land="lightgreen",
    #water="lightblue",
    projection="M16c",
)

# Plot point data
f.plot(x=example_df.x, y=example_df.y, style="c0.3c", fill="white", pen="black")

# f.show()
f.savefig(out_plot)
