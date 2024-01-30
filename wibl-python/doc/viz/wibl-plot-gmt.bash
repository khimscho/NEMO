#!/usr/bin/env zsh
set -e

NAME_STEM=$1
SOUNDINGS_FILE=$2
GEBCO_FILE=$3

SOUNDINGS_EXT=json
SOUNDINGS_RAST=${SOUNDINGS_FILE/\.$SOUNDINGS_EXT/\.tif}
FORMATS='pdf,png'
RASTER_NODATA=-99999
RASTER_RES=0.0005
# TODO: make bounds buffer dynamic based on scale of soundings
BOUNDS_BUFFER=0.5
INSET_BUFFER_INC=$((4*$BOUNDS_BUFFER-$BOUNDS_BUFFER))
# TODO: allow width to be specified
PROJECTION=M6i

# Generate GeoTIFF of soundings from GeoJSON file
echo "Generate GeoTIFF of soundings from GeoJSON file..."
gdal_rasterize -q -a_srs EPSG:4326 -tr $RASTER_RES $RASTER_RES \
  -a_nodata $RASTER_NODATA -co COMPRESS=DEFLATE -co ZLEVEL=9 -a depth \
  $SOUNDINGS_FILE $SOUNDINGS_RAST

# Read bounds from rasterized soundings and set environment buffered variables for bounds
echo "Read bounds from rasterized soundings and set environment buffered variables for bounds..."
gdalinfo -json $SOUNDINGS_RAST | \
  jq '
    @sh "GMT_PLOT_XMIN=$((\(.wgs84Extent.coordinates[0][0][0]) - $BOUNDS_BUFFER))",
    @sh "GMT_PLOT_XMAX=$((\(.wgs84Extent.coordinates[0][2][0]) + $BOUNDS_BUFFER))",
    @sh "GMT_PLOT_YMIN=$((\(.wgs84Extent.coordinates[0][1][1]) - $BOUNDS_BUFFER))",
    @sh "GMT_PLOT_YMAX=$((\(.wgs84Extent.coordinates[0][0][1]) + $BOUNDS_BUFFER))",
    @sh "GMT_PLOT_CENT_LON=\(.cornerCoordinates.center[0])",
    @sh "GMT_PLOT_CENT_LAT=\(.cornerCoordinates.center[1])"
  ' | while read cmd; do eval "export $cmd"; done
GLOBAL_INSET_PROJ="G$GMT_PLOT_CENT_LON/$GMT_PLOT_CENT_LAT/?"
REGION="$GMT_PLOT_XMIN/$GMT_PLOT_XMAX/$GMT_PLOT_YMIN/$GMT_PLOT_YMAX"
# Calculate region for overview inset map
GMT_PLOT_XMIN_INSET=$(($GMT_PLOT_XMIN - $INSET_BUFFER_INC))
GMT_PLOT_XMAX_INSET=$(($GMT_PLOT_XMAX + $INSET_BUFFER_INC))
GMT_PLOT_YMIN_INSET=$(($GMT_PLOT_YMIN - $INSET_BUFFER_INC))
GMT_PLOT_YMAX_INSET=$(($GMT_PLOT_YMAX + $INSET_BUFFER_INC))
REGION_INSET="$GMT_PLOT_XMIN_INSET/$GMT_PLOT_XMAX_INSET/$GMT_PLOT_YMIN_INSET/$GMT_PLOT_YMAX_INSET"
REGION_POLY="${NAME_STEM}-region.txt"
cat << EOF > $REGION_POLY
$GMT_PLOT_XMIN $GMT_PLOT_YMIN $GMT_PLOT_XMAX $GMT_PLOT_YMAX
EOF
INSET_POLY="${NAME_STEM}-inset.txt"
cat << EOF > $INSET_POLY
$GMT_PLOT_XMIN_INSET $GMT_PLOT_YMIN_INSET $GMT_PLOT_XMAX_INSET $GMT_PLOT_YMAX_INSET
EOF

# Create map
echo "Creating map for soundings ${SOUNDINGS_FILE}..."
gmt begin ${NAME_STEM} ${FORMATS} I+m.2c
  # Plot GEBCO bathymetry and corresponding colorbar
  gmt grdimage $GEBCO_FILE -J$PROJECTION -R$REGION -Cterra
  # TODO: Change -Bx50... (width of scalebar in km) to be dynamic based on extent
  #   see if gmtmath is useful for this: https://docs.generic-mapping-tools.org/6.4/gmtmath.html
  gmt colorbar -DJBC+o0c/0.8c -Bx50+l'GEBCO 2023 Bathymetry' -By+lm
  # Add frame around entire map with title
  gmt basemap -J$PROJECTION -R$REGION -B+t"Soundings from '$SOUNDINGS_FILE'" -B \
    -LjBR+o0.75c/0.5c+w100k+f+u --FONT_TITLE=20p,Helvetica
  # Plot soundings
  gmt grdview ${SOUNDINGS_RAST}
  # TODO: Add color map and colorbar for soundings
  # Plot global inset
  gmt inset begin -DjTR+w2c+o0.25c/0.25c -F+gwhite+p1p+c0.1c
		gmt coast -Rg -J$GLOBAL_INSET_PROJ -Da -Ggray -A5000 -Bg
		gmt plot $INSET_POLY -J$GLOBAL_INSET_PROJ -Sr+s -W0.5p,blue
	gmt inset end
  # Plot region inset
  gmt inset begin -DjTR+w2c/2.2c+o0.25c/2.55c -F+gwhite+p1p+c0.1c
    gmt coast -R$REGION_INSET -JM2c -Swhite -Ggrey --MAP_FRAME_TYPE=plain
    gmt plot $REGION_POLY -J'M?' -Sr+s -W0.5p,blue
  gmt inset end
gmt end
