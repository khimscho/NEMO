# DCDB will issue you with a unique provider ID to identify your uploads.  This is
# used in many different places in the code; set it here.
DCDB_PROVIDER_ID=# Six-character DCDB identifier

# Where you want the configuration file generated
CONFIG_FILE_DIR=# Specify a local full path

# Where you talk to the management server.  Leave blank if you don't want to run
# the management console.
MANAGEMENT_URL=# URL for the server running the REST interface for metadata management

# Notification endpoints.  These are typically the message topics that you notify
# when various stages of the processing complete.
NOTE_CREATED=# Topic endpoint when a new file is created in the incoming bucket
NOTE_CONVERTED=# Topic endpoint when a WIBL file is successfully converted to GeoJSON
NOTE_VALIDATED=# Topic endpoint when a GeoJSON file's metadata is validated
NOTE_UPLOADED=# Topic endpoint when a GeoJSON file is successfully uploaded to DCDB

# Below here you shouldn't probably modify

DCDB_UPLOAD_URL=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/
PROVIDER_PREFIX=`echo ${DCDB_PROVIDER_ID} | tr '[:upper:]' '[:lower:]'`
STAGING_BUCKET=${PROVIDER_PREFIX}-wibl-staging
