from pathlib import Path

import pygmt
import xarray

out_plot: Path = Path('pygmt_test.pdf')
if out_plot.exists():
    out_plot.unlink()

gebco_path: str = '/tmp/gmtout/GEBCO_2023.nc'
gebco_ds: xarray.Dataset = xarray.open_dataset(gebco_path)
gebco_el: xarray.DataArray = gebco_ds['elevation']
# gebco_el = pygmt.load_dataarray(f"{gebco_path}?elevation")
region = [-71.0, -70.0, 42.5, 43.5]

f = pygmt.Figure()
f.grdimage(gebco_el,
           region=region,
           projection="M16c",
           frame="afg",
           cmap=True)

f.colorbar(position="JML+o1.4c/0c+w7c/0.5c", frame=["xa1000f500+lElevation", "y+lm"])

f.coast(
    region=region,
    shorelines=True,
    land="lightgreen",
    #water="lightblue",
    projection="M16c",
)

# f.show()
f.savefig(out_plot)
