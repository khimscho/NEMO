#!/usr/bin/env zsh
set -e

NAME_STEM=$1
SOUNDINGS_FILE=$2
GEBCO_FILE=$3

COLOR_TABLE="${SOUNDINGS_FILE}.cpt"
FORMATS='pdf,png'
# TODO: make bounds buffer dynamic based on scale of soundings
BOUNDS_BUFFER=0.5
INSET_BUFFER_INC=$((4*$BOUNDS_BUFFER-$BOUNDS_BUFFER))
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
# Calculate region for overview inset map
GMT_PLOT_XMIN_INSET=$(($GMT_PLOT_XMIN - $INSET_BUFFER_INC))
GMT_PLOT_XMAX_INSET=$(($GMT_PLOT_XMAX + $INSET_BUFFER_INC))
GMT_PLOT_YMIN_INSET=$(($GMT_PLOT_YMIN - $INSET_BUFFER_INC))
GMT_PLOT_YMAX_INSET=$(($GMT_PLOT_YMAX + $INSET_BUFFER_INC))
REGION_INSET="$GMT_PLOT_XMIN_INSET/$GMT_PLOT_XMAX_INSET/$GMT_PLOT_YMIN_INSET/$GMT_PLOT_YMAX_INSET"
REGION_POLY="${NAME_STEM}-inset.txt"
cat << EOF > $REGION_POLY
$GMT_PLOT_XMIN $GMT_PLOT_YMIN $GMT_PLOT_XMAX $GMT_PLOT_YMAX
EOF

# Plot
gmt begin ${NAME_STEM} ${FORMATS}
  gmt grdimage ${GEBCO_FILE} -J$PROJECTION -R$REGION -Cterra
  gmt colorbar -DJBC+o0c/0.8c -Bx50+l'GEBCO Bathymetry' -By+lm
  # TODO: Change +w50k (width of scalebar in km) to be dynamic based on extent
  #   see if gmtmath is useful for this: https://docs.generic-mapping-tools.org/6.4/gmtmath.html
  gmt basemap -J$PROJECTION -R$REGION -B -LjBR+o0.75c/0.5c+w100k+f+u
  gmt grdimage ${SOUNDINGS_FILE} -C${COLOR_TABLE}
  gmt inset begin -DjTR+w2c/2.2c+o0.25c/0.25c -F+gwhite+p1p+c0.1c
    gmt coast -R$REGION_INSET -J'M2c' -Swhite -Ggrey --MAP_FRAME_TYPE=plain
    gmt plot $REGION_POLY -J'M?' -Sr+s -W0.5p,blue
  gmt inset end
gmt end
