#!/bin/bash
#
# This sets up AWS S3 buckets for the incoming WIBL data, and for the staging location
# for intermediate files before they get sent to DCDB.

source configuration-parameters.sh

echo $'\e[31m' Creating incoming data bucket as ${INCOMING_BUCKET} in region ${AWS_REGION} ... $'\e[0m'
aws s3api create-bucket \
    --bucket ${INCOMING_BUCKET} \
    --region ${AWS_REGION} \
    --create-bucket-configuration LocationConstraint=${AWS_REGION}

echo $'\e[31m' Creating staging data bucket as ${STAGING_BUCKET} in region ${AWS_REGION} ... $'\e[0m'
aws s3api create-bucket \
    --bucket ${STAGING_BUCKET} \
    --region ${AWS_REGION} \
    --create-bucket-configuration LocationConstraint=${AWS_REGION}
