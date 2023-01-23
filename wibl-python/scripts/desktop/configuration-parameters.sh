# DCDB will issue you with a unique provider ID to identify your uploads.  This is
# used in many different places in the code; set it here.
DCDB_PROVIDER_ID=# Six-character DCDB identifier

# Where you want the configuration file generated
CONFIG_FILE_DIR=# Specify a local full path

# Below here you shouldn't probably modify

DCDB_UPLOAD_URL=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/
PROVIDER_PREFIX=`echo ${DCDB_PROVIDER_ID} | tr '[:upper:]' '[:lower:]'`
STAGING_BUCKET=${PROVIDER_PREFIX}-wibl-staging
