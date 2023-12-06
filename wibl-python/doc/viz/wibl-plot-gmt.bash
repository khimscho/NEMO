#!/usr/bin/env zsh
set -e

NAME_STEM=$1
SOUNDINGS_FILE=$2
GEBCO_FILE=$3

COLOR_TABLE="${SOUNDINGS_FILE}.cpt"
FORMATS='pdf,png'
# TODO: make dynamic based on scale of soundings
BOUNDS_BUFFER=0.5
# TODO: allow width to be specified
PROJECTION=M6i

# TODO: Take GeoJSON file as input and use gdal_rasterize to generate GeoTIFF of soundings
# gdal_rasterize -a_srs EPSG:4326 -tr 0.0005 0.0005 -a_nodata -99999 -co COMPRESS=DEFLATE -co ZLEVEL=9 -a depth bigfile-md.json bigfile-md.tif

# Make color table for soundings
# TODO: Figure out how to skip nodata values in color table
gmt grd2cpt ${SOUNDINGS_FILE} -N > ${COLOR_TABLE}

# Read bounds from rasterized soundings and set environment buffered variables for bounds
gdalinfo -json $SOUNDINGS_FILE | \
  jq '
    @sh "GMT_PLOT_XMIN=$((\(.wgs84Extent.coordinates[0][0][0]) - $BOUNDS_BUFFER))",
    @sh "GMT_PLOT_XMAX=$((\(.wgs84Extent.coordinates[0][2][0]) + $BOUNDS_BUFFER))",
    @sh "GMT_PLOT_YMIN=$((\(.wgs84Extent.coordinates[0][1][1]) - $BOUNDS_BUFFER))",
    @sh "GMT_PLOT_YMAX=$((\(.wgs84Extent.coordinates[0][0][1]) + $BOUNDS_BUFFER))"
  ' | tee | while read cmd; do eval "export $cmd"; done
REGION="$GMT_PLOT_XMIN/$GMT_PLOT_XMAX/$GMT_PLOT_YMIN/$GMT_PLOT_YMAX"

# Plot
gmt begin ${NAME_STEM} ${FORMATS}
  gmt grdimage ${GEBCO_FILE} -J$PROJECTION -R$REGION -Cterra
  gmt colorbar -DJBC+o0c/0.8c -Bx100+l'GEBCO Bathymetry' -By+lm
  # TODO: Change +w100k (width of scalebar in km) to be dynamic based on extent
  #   see if gmtmath is useful for this: https://docs.generic-mapping-tools.org/6.4/gmtmath.html
  gmt basemap -J$PROJECTION -R$REGION -B -LjBR+o0.75c/0.5c+w100k+f+u
  gmt grdimage ${SOUNDINGS_FILE} -C${COLOR_TABLE}
  # TODO: Add inset
gmt end
