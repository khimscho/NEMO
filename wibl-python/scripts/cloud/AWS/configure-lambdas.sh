#!/bin/bash
#
# This sets up the permissions, triggers, and lambdas required for the WIBL code to run in
# AWS.  If, of course, you have part of this set up already, then you're likely to encounter
# difficulties.  Probably best to clean up in the console first, then try again.

source configuration-parameters.sh

####################
# Phase 1: Package up the WIBL software
#

echo $'\e[31mBuilding the WIBL package from' ${WIBL_SRC_LOCATION} $'...\e[0m'

cat >package-setup.sh <<-HERE
cd /build
pip install --target ./package -r requirements-lambda.txt
pip install --target ./package --no-deps .
HERE

# Remove any previous packaging attempt, so it doesn't try to update what's there already
rm -r ${WIBL_SRC_LOCATION}/package

# Package up the WIBL software so that it can be deployed as a ZIP file
cat package-setup.sh | \
    docker run --mount type=bind,source=${WIBL_SRC_LOCATION},target=/build \
	    -i --entrypoint /bin/bash public.ecr.aws/lambda/python:${PYTHONVERSION}-${ARCHITECTURE}

# You may not be able to run these commands inside the docker container, since the
# AWS container doesn't have ZIP.  If that's the case, there should be a ./package
# directory in your WIBL distribution (from the container); zip the internals of that
# instead.
WIBL_PACKAGE=${WIBL_BIN_LOCATION}/wibl-package-py${PYTHONVERSION}-${ARCHITECTURE}.zip
rm -rf ${WIBL_PACKAGE}
(cd ${WIBL_SRC_LOCATION}/package;
zip -q -r ${WIBL_PACKAGE} .)

# Clean up packaging attempt so that it doesn't throw off source code control tools
rm -r ${WIBL_SRC_LOCATION}/package

#####################
# Phase 2: Generate IAM roles for the processing and submission roles, add policy support
#

echo $'\e[31mBuilding the IAM roles for processing,' ${PROCESSING_LAMBDA_ROLE} $', and submission,' ${SUBMISSION_LAMBDA_ROLE} $', lambdas ...\e[0m'

cat >trust-policy.json <<-HERE
{
        "Version":      "2012-10-17",
        "Statement":    [
                {
                        "Effect": "Allow",
                        "Principal": {
                                "Service": ["lambda.amazonaws.com"]
                        },
                        "Action": "sts:AssumeRole"
                }
        ]
}
HERE

# Generate roles that allow trust for Lambdas, one each for processing & submission
aws iam create-role \
	--role-name ${PROCESSING_LAMBDA_ROLE} \
	--assume-role-policy-document file://trust-policy.json
aws iam create-role \
	--role-name ${SUBMISSION_LAMBDA_ROLE} \
	--assume-role-policy-document file://trust-policy.json

# Attach the ability to run Lambdas to these roles
aws iam attach-role-policy \
	--role-name ${PROCESSING_LAMBDA_ROLE} \
	--policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole
aws iam attach-role-policy \
	--role-name ${SUBMISSION_LAMBDA_ROLE} \
	--policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole

echo $'\e[31mWaiting for 10 seconds to allow roles to propagate ...\e[0m'
sleep 10

########################
# Phase 3: Generate the processing lambda, and configure it to trigger from the S3 incoming bucket
#
# Create the processing lambda

echo $'\e[31mGenerating the processing lambda as' ${PROCESSING_LAMBDA} $'...\e[0m'

aws lambda create-function \
    --function-name ${PROCESSING_LAMBDA} \
	--role arn:aws:iam::${ACCOUNT_NUMBER}:role/${PROCESSING_LAMBDA_ROLE} \
	--runtime python${PYTHONVERSION} \
    --timeout ${LAMBDA_TIMEOUT} \
    --memory-size ${LAMBDA_MEMORY} \
	--handler wibl.processing.cloud.aws.lambda_function.lambda_handler \
	--zip-file fileb://${WIBL_PACKAGE} \
	--architectures ${ARCHITECTURE} \
	--layers ${SCIPY_LAYER_NAME} \
	--environment "Variables={PROVIDER_ID=${DCDB_PROVIDER_ID},STAGING_BUCKET=${STAGING_BUCKET},UPLOAD_POINT=${DCDB_UPLOAD_URL}}"

# We now have to configure the processing lambda to take events from the incoming S3 bucket,
# and for uploads to that bucket to trigger the lambda
UUID=`uuidgen`
cat >bucket-notification-cfg.json <<-HERE
{
    "LambdaFunctionConfigurations": [
        {
            "Id": "${UUID}",
            "LambdaFunctionArn": "arn:aws:lambda:${AWS_REGION}:${ACCOUNT_NUMBER}:function:${PROCESSING_LAMBDA}",
            "Events": [
                "s3:ObjectCreated:Put",
                "s3:ObjectCreated:CompleteMultipartUpload"
            ]
        }
    ]
}
HERE
cat >lambda-s3-access.json <<-HERE
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "LambdaAllowS3Access",
            "Effect": "Allow",
            "Action": [
                "s3:*"
            ],
            "Resource": [
                "arn:aws:s3:::${INCOMING_BUCKET}",
                "arn:aws:s3:::${INCOMING_BUCKET}/*",
                "arn:aws:s3:::${STAGING_BUCKET}",
                "arn:aws:s3:::${STAGING_BUCKET}/*"
            ]
        }
    ]
}
HERE

echo $'\e[31mAdding permissions to the processing lambda for S3 invocation ...\e[0m'

aws lambda add-permission \
    --function-name ${PROCESSING_LAMBDA} \
	--action lambda:InvokeFunction \
	--statement-id s3invoke \
	--principal s3.amazonaws.com \
	--source-arn arn:aws:s3:::${INCOMING_BUCKET} \
	--source-account ${ACCOUNT_NUMBER}

echo $'\e[31mWaiting 5 seconds for lambda creation to complete ...\e[0m'
sleep 5

echo $'\e[31mAdding bucket notification trigger to' ${INCOMING_BUCKET} $'for processing lambda ...\e[0m'

aws s3api put-bucket-notification-configuration \
	--bucket ${INCOMING_BUCKET} \
	--notification-configuration file://bucket-notification-cfg.json

echo $'\e[31mConfiguring S3 access policy to the role so that lambda can access S3 incoming bucket ...\e[0m'

aws iam put-role-policy \
	--role-name ${PROCESSING_LAMBDA_ROLE} \
	--policy-name lambda-s3-access \
	--policy-document file://lambda-s3-access.json

exit 0

########################
# Phase 4: Generate the submission lambda, and configure it to trigger from the S3 staging bucket
#
# Create the submission bucket
aws lambda create-function \
	--function-name ${SUBMISSION_LAMBDA} \
	--role arn:aws:iam::${ACCOUNT_NUMBER}:role/${SUBMISSION_LAMBDA_ROLE} \
	--runtime python${PYTHONVERSION} \
    --timeout ${LAMBDA_TIMEOUT} \
    --memory-size ${LAMBDA_MEMORY} \
	--handler wibl.submission.cloud.aws.lambda_function.lambda_handler \
	--zip-file fileb://${WIBL_PACKAGE} \
	--environment "Variables={PROVIDER_ID=${DCDB_PROVIDER_ID},PROVIDER_AUTH=${DCDB_AUTH_KEY},STAGING_BUCKET=${STAGING_BUCKET},UPLOAD_POINT=${DCDB_UPLOAD_URL}}"

UUID=`uuidgen`
cat >bucket-notification-cfg.json <<-HERE
{
    "LambdaFunctionConfigurations": [
        {
            "Id": "${UUID}",
            "LambdaFunctionArn": "arn:aws:lambda:${AWS_REGION}:${ACCOUNT_NUMBER}:function:${SUBMISSION_LAMBDA}",
            "Events": [
                "s3:ObjectCreated:Put",
                "s3:ObjectCreated:CompleteMultipartUpload"
            ]
        }
    ]
}
HERE
cat >lambda-s3-access.json <<-HERE
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "LambdaAllowS3Access",
            "Effect": "Allow",
            "Action": [
                "s3:*"
            ],
            "Resource": [
                "arn:aws:s3:::${STAGING_BUCKET}",
                "arn:aws:s3:::${STAGING_BUCKET}/*"
            ]
        }
    ]
}
HERE

aws lambda add-permission \
    --function-name ${SUBMISSION_LAMBDA} \
	--action lambda:InvokeFunction \
	--statement-id s3invoke \
	--principal s3.amazonaws.com \
	--source-arn arn:aws:s3:::${STAGING_BUCKET} \
	--source-account ${ACCOUNT_NUMBER}

aws s3api put-bucket-notification-configuration \
	--bucket ${STAGING_BUCKET} \
	--notification-configuration file://bucket-notification-cfg.json

aws iam put-role-policy \
	--role-name ${SUBMISSION_LAMBDA_ROLE} \
	--policy-name lambda-s3-access \
	--policy-document file://lambda-s3-access.json
