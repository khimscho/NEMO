# This provides information on the configuration of the lambdas
# being generated, the specification for the buckets being used,
# roles, and so on.  This is a separate file so that it can be
# sourced by code to set up lambdas, and another to set up the
# buckets.

# Set this to your AWS account number
ACCOUNT_NUMBER=# Twelve-digit AWS account number

# DCDB will issue you with a unique provider ID to identify your uploads.  This is
# used in many different places in the code; set it here.
DCDB_PROVIDER_ID=# Six-character DCDB identifier

# The script needs to know where to find the WIBL source code so that it can package
# the Python library for upload to AWS.  It also needs a place to stash the resulting
# package.  Set those locations here.
WIBL_SRC_LOCATION=${HOME}/Projects/WIBL/wibl-python
WIBL_BIN_LOCATION=${HOME}/Projects-Extras/WIBL/AWS-Setup

# DCDB should provide you with a token to authorise you to upload; change this code
# so that it can find where you've stashed it, and read it in to allow for setup of
# the submission Lambda.
AUTHKEY=`cat ../DCDB-Upload-Tokens/ingest-external-${DCDB_PROVIDER_ID}.txt`

# These parameters configure the AWS region and technical details of the Lambda runtime
# that will be used.  If you change the region, you will also want to change the SciPy
# layer name to reflect your local version.  You can change the architecture and Python
# version, but note that not all combinations of these will result in a Lambda that can
# both get the SciPy later that it needs, and boot successfully.
AWS_REGION=us-east-2
ARCHITECTURE=x86_64
PYTHONVERSION=3.8
SCIPY_LAYER_NAME=arn:aws:lambda:us-east-2:259788987135:layer:AWSLambda-Python38-SciPy1x:107

#---------------------------------------------------------------------------------------
# Below here you probably don't need to change much

DCDB_UPLOAD_URL=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/

PROVIDER_PREFIX=`echo ${DCDB_PROVIDER_ID} | tr '[:upper:]' '[:lower:]'`

PROCESSING_LAMBDA=${PROVIDER_PREFIX}-wibl-processing
SUBMISSION_LAMBDA=${PROVIDER_PREFIX}-wibl-submission
PROCESSING_LAMBDA_ROLE=${PROCESSING_LAMBDA}-lambda
SUBMISSION_LAMBDA_ROLE=${SUBMISSION_LAMBDA}-lambda

LAMBDA_TIMEOUT=30
LAMBDA_MEMORY=2048

INCOMING_BUCKET=${PROVIDER_PREFIX}-wibl-incoming
STAGING_BUCKET=${PROVIDER_PREFIX}-wibl-staging
