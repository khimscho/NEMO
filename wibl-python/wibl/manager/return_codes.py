##\file return_codes.py
# \brief Magic numbers for services and status codes
#
# Encapsulate (as Enums) special codes for the REST API to return, and for other
# code to indicate processing and upload status.
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

from enum import Enum

class ReturnCodes(Enum):
    OK = 200
    RECORD_CREATED = 201
    RECORD_DELETED = 204
    FILE_NOT_FOUND = 404
    RECORD_CONFLICT = 409

class ProcessingStatus(Enum):
    PROCESSING_STARTED = 0
    PROCESSING_SUCCESSFUL = 1
    PROCESSING_FAILED = 2

class UploadStatus(Enum):
    UPLOAD_STARTED = 0
    UPLOAD_SUCCESSFUL = 1
    UPLOAD_FAILED = 2
