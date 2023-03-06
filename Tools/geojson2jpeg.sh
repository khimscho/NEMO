#!/bin/bash
#
# This script is an experimental interface between GeoJSON files as produced by the WIBL
# processing system (and prefered by DCDB) and a map product that can be used to provide
# feedback to data collectors.  It requires both GDAL (https://gdal.org) and GMT
# (https://www.generic-mapping-tools.org) installed to operate (as well as a functioning
# Unix-like environment for bash, sed, find, awk, cat, etc.).  The code to do plotting
# currently uses the GEBCO global grid (https://www.gebco.net/data_and_products/gridded_bathymetry_data/)
# for background, preferring the NetCDF file.  You should expect that in addition to
# editing the inputs (lines 35-39) before running the script, you may have to adjust the
# script to give you exactly the plot that you want.
#
# Copyright 2023 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
# Hydrographic Center, University of New Hampshire.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.

# Set some environmentals before the script runs
inputdir='/home/mapper/GeoJSON' ; # Directory to where the JSON files are
outputdir='/home/mapper/GeoJSON/output' ; # Output directory
cell=0.0005 ; # Cell size of the grid created from the xyz file
buffer=0.01 ; # buffer size (in decimal degrees) around the edge of the plot
plotname='TestPlot'

# Cycle through each GeoJSON and if it hasn't been converted to XYZ process it
cd $inputdir 
find -name "*.json" -type f  | sed "s/\.\///g" | awk '{print inputdir"/"$1}' inputdir=$inputdir > $outputdir/tmp.list
cd $outputdir
while IFS= read -r line
do
    file="$(basename $line .json)"
    if [ -f "$outputdir/$file.xyz" ]; then
        printf 'File %s already converted...\n' "$line"
    else
        echo $file.xyz
        printf 'Working on %s file...\n' "$line"
        ogr2ogr -f csv -lco GEOMETRY=AS_WKT test.csv $line
        cat test.csv | sed 's/"//g' | sed 's/(//g' | sed 's/)//g' | sed 's/,/ /g' | awk '{if (NR > 2) print $2,$3,$4*-1}' > $file.xyz
    fi
done < tmp.list
rm tmp.list test.csv

# Plot the data
rm -f $testplot.*
bounds=`cat *.xyz | gmt gmtinfo -C | awk '{printf "%.6f %.6f\n%.6f %.6f\n%.6f %.6f\n%.6f %.6f\n", $1-buffer,$3-buffer,$1-buffer,$3-buffer,$2+buffer,$4+buffer,$2+buffer,$4-buffer}' buffer=$buffer | gmt gmtinfo  -I$cell`
orient=`cat *.xyz | gmt gmtinfo -C | awk '{if (($2-$1) >= ($4-$3)) print "-P"}'`
cat *.xyz | awk '{if ($3 >= -9000) print $0}' | gmt xyz2grd $bounds -I$cell -V -G$plotname.grd
gmt grd2cpt -Chaxby $plotname.grd -Z > haxbyc.cpt
lines=`wc -l haxbyc.cpt | awk '{print $1-3}'`

echo orient = $orient
if [ "$orient" = "-P" ]; then
    echo "Portrait Plot"
    gmt grdimage GEBCO_2022.nc -Cterra -K -JM6i $bounds -P -Y2.25 > $plotname.ps
    gmt grdimage $plotname.grd -JM $orient -Ba5mg5m  -V -Chaxbyc.cpt -O  -Q -P -K >> $plotname.ps
    gmt psscale -Chaxbyc.cpt -O -Dx3i/0.25i+w5i/0.25i+jTC+h -Y-1.5 -Ba20 >> $plotname.ps
else
    echo "Landscape Plot"
    gmt grdimage GEBCO_2022.nc -Cterra -K -JM9i $bounds > $plotname.ps
     gmt grdimage $plotname.grd -JM $orient -Ba5mg5m  -V -Chaxbyc.cpt -O  -Q -P -K >> $plotname.ps
    gmt psscale -Chaxbyc.cpt -O -Dx4.5i/0.25i+w5i/0.25i+jTC+h -Y-1.5 -Ba20 >> $plotname.ps
fi

gmt psconvert $plotname.ps -A -Tj

