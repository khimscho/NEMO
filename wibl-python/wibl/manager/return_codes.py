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
