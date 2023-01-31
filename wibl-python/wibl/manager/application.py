from flask import Flask, abort
from flask_restful import Api, Resource, reqparse, fields, marshal_with
from flask_sqlalchemy import SQLAlchemy
from typing import NoReturn
from datetime import datetime, timezone

from return_codes import ReturnCodes, ProcessingStatus, UploadStatus

app = Flask('WIBL-Manager')
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///database.db'
db = SQLAlchemy(app)
class WIBLDataModel(db.Model):
    """
    Data model for WIBL file metadata during processing, held in a suitable database (controlled externally)
    """
    fileid = db.Column(db.String(40), primary_key=True)
    processtime = db.Column(db.String(30))
    updatetime = db.Column(db.String(30))
    notifytime = db.Column(db.String(30))
    logger = db.Column(db.String(80))
    platform = db.Column(db.String(80))
    size = db.Column(db.Float, nullable=False)
    observations = db.Column(db.Integer)
    soundings = db.Column(db.Integer)
    starttime = db.Column(db.String(30))
    endtime = db.Column(db.String(30))
    status = db.Column(db.Integer)

    def __repr__(self):
        return f'file {self.fileid} at {self.processtime} for logger {self.logger} on {self.platform} size {self.size} MB, status={self.status}.'

# Data model for GeoJSON metadata stored in the database
class GeoJSONDataModel(db.Model):
    fileid = db.Column(db.String(40), primary_key=True)
    uploadtime = db.Column(db.String(30))
    updatetime = db.Column(db.String(30))
    notifytime = db.Column(db.String(30))
    logger = db.Column(db.String(80))
    size = db.Column(db.Float, nullable=False)
    soundings = db.Column(db.Integer)
    status = db.Column(db.Integer)

    def __repr__(self):
        return f'file {self.fileid} at {self.uploadtime} for logger {self.logger} with {self.soundings} observations, status={self.status}'

#with app.app_context():
#    db.create_all()

# Request parser to handle POST requests on the WIBL end-point (which require only a size to start with)
WIBL_Args = reqparse.RequestParser()
WIBL_Args.add_argument('size', type=float, help='Size of the WIBL file in MB.', required=True)

# Request parser to hanndle PUT requests on an existing WIBL metadata record, after processing is completed
WIBL_Update_Args = reqparse.RequestParser()
WIBL_Update_Args.add_argument('logger', type=str, help='Logger name (unique ID) value.')
WIBL_Update_Args.add_argument('platform', type=str, help='Name of the observing platform.')
WIBL_Update_Args.add_argument('size', type=float, help='Size of the WIBL file in MB.')
WIBL_Update_Args.add_argument('observations', type=int, help='Number of soundings in the file.')
WIBL_Update_Args.add_argument('soundings', type=int, help='Number of soundings after processing.')
WIBL_Update_Args.add_argument('startTime', type=str, help='First output sounding time.')
WIBL_Update_Args.add_argument('endTime', type=str, help='Last output sounding time.')
WIBL_Update_Args.add_argument('notifyTime', type=str, help='Time of processing failure notification.')
WIBL_Update_Args.add_argument('status', type=int, help='Status of conversion code run.')

# Data dictionary for marshalling the objects returned from the database (WIBLDataModel) into JSON
wibl_resource_fields = {
    'fileid':           fields.String,
    'processtime':      fields.String,
    'updatetime':       fields.String,
    'notifytime':       fields.String,
    'logger':           fields.String,
    'platform':         fields.String,
    'size':             fields.Float,
    'observations':     fields.Integer,
    'soundings':        fields.Integer,
    'starttime':        fields.String,
    'endtime':          fields.String,
    'status':           fields.Integer
}

class WIBLData(Resource):
    """
    RESTful end-point for manipulating the WIBL data file database component.
    """
    @marshal_with(wibl_resource_fields)
    def get(self, fileid):
        """
        Lookup for a single file's metadata.
        :param fileid:  Filename to look up (typically a UUID)
        :return: Metadata instance for the file or abort(NOT_FOUND)
        """
        if fileid == 'all':
            result = db.session.query(WIBLDataModel).all()
        else:
            result = WIBLDataModel.query.filter_by(fileid=fileid).first()
        if not result:
            abort(ReturnCodes.FILE_NOT_FOUND.value, description='That WIBL file does not exist.')
        return result

    @marshal_with(wibl_resource_fields)
    def post(self, fileid):
        result = WIBLDataModel.query.filter_by(fileid=fileid).first()
        if result:
            abort(ReturnCodes.RECORD_CONFLICT.value, message='That WIBL file already exists in the database; use PUT to update.')
        args = WIBL_Args.parse_args()
        timestamp = datetime.now(timezone.utc).isoformat()
        wibl_file = WIBLDataModel(fileid=fileid, processtime=timestamp,
                                  logger='Unknown', platform='Unknown', size=args['size'],
                                  observations=-1, soundings=-1, starttime='Unknown', endtime='Unknown',
                                  status=ProcessingStatus.PROCESSING_STARTED.value)
        db.session.add(wibl_file)
        db.session.commit()
        return wibl_file, ReturnCodes.RECORD_CREATED.value
    
    @marshal_with(wibl_resource_fields)
    def put(self, fileid):
        args = WIBL_Update_Args.parse_args()
        wibl_file = WIBLDataModel.query.filter_by(fileid=fileid).first()
        if not wibl_file:
            abort(ReturnCodes.FILE_NOT_FOUND.value, description='That WIBL file does not exist in database; use POST to add.')
        timestamp = datetime.now(timezone.utc).isoformat()
        wibl_file.updatetime = timestamp
        if args['notifyTime']:
            wibl_file.notifytime = args['notifyTime']
        if args['logger']:
            wibl_file.logger = args['logger']
        if args['platform']:
            wibl_file.platform = args['platform']
        if args['size']:
            wibl_file.size = args['size']
        if args['observations']:
            wibl_file.observations = args['observations']
        if args['soundings']:
            wibl_file.soundings = args['soundings']
        if args['startTime']:
            wibl_file.starttime = args['startTime']
        if args['endTime']:
            wibl_file.endtime = args['endTime']
        if args['status']:
            wibl_file.status = args['status']
        db.session.commit()
        return wibl_file, ReturnCodes.RECORD_CREATED.value

    def delete(self, fileid):
        wibl_file = WIBLDataModel.query.filter_by(fileid=fileid).first()
        if not wibl_file:
            abort(ReturnCodes.FILE_NOT_FOUND.value, description='That WIBL file does not exist in the database, and therefore cannot be deleted.')
        db.session.delete(wibl_file)
        db.session.commit()

        return ReturnCodes.RECORD_DELETED.value

# Arguments passable to the GeoJSON end-point to POST a new record's metadata
GeoJSON_Args = reqparse.RequestParser()
GeoJSON_Args.add_argument('size', type=float, help='Size of the GeoJSON file in MB.', required=True)

# Arguments passable to the GeoJSON end-point for PUT updates to an existing file's metadata
GeoJSON_Update_Args = reqparse.RequestParser()
GeoJSON_Update_Args.add_argument('notifyTime',type=str, help='Time of upload failure notification.')
GeoJSON_Update_Args.add_argument('logger', type=str, help='Logger name (unique ID) value.')
GeoJSON_Update_Args.add_argument('size', type=float, help='Size of the GeoJSON file in MB.')
GeoJSON_Update_Args.add_argument('soundings', type=int, help='Number of soundings in the file.')
GeoJSON_Update_Args.add_argument('status', type=int, help='Status of archive upload attempt.')

# Data mapping for marshalling GeoJSONDataModel for transfer
geojson_resource_fields = {
    'fileid':       fields.String,
    'uploadtime':   fields.String,
    'updatetime':   fields.String,
    'notifytime':   fields.String,
    'logger':       fields.String,
    'size':         fields.Float,
    'soundings':    fields.Integer,
    'status':       fields.Integer
}

class GeoJSONData(Resource):
    @marshal_with(geojson_resource_fields)
    def get(self, fileid):
        if fileid == 'all':
            result = db.session.query(GeoJSONDataModel).all()
        else:
            result = GeoJSONDataModel.query.filter_by(fileid=fileid).first()
        if not result:
            abort(ReturnCodes.FILE_NOT_FOUND.value, description='That GeoJSON file does not exist.')
        return result

    @marshal_with(geojson_resource_fields)
    def post(self, fileid):
        result = GeoJSONDataModel.query.filter_by(fileid=fileid).first()
        if result:
            abort(ReturnCodes.RECORD_CONFLICT.value, message='That GeoJSON file already exists in the database; use PUT to update.')
        args = GeoJSON_Args.parse_args()
        timestamp = datetime.now(timezone.utc).isoformat()
        geojson_file = GeoJSONDataModel(fileid=fileid, uploadtime=timestamp,
                                        logger='Unknown', size=args['size'],
                                        soundings=-1, status=UploadStatus.UPLOAD_STARTED.value)
        db.session.add(geojson_file)
        db.session.commit()
        return geojson_file, ReturnCodes.RECORD_CREATED.value

    @marshal_with(geojson_resource_fields)
    def put(self, fileid):
        args = GeoJSON_Update_Args.parse_args()
        geojson_file = GeoJSONDataModel.query.filter_by(fileid=fileid).first()
        if not geojson_file:
            abort(ReturnCodes.FILE_NOT_FOUND.value, description='That GeoJSON file does not exist in database; use POST to add.')
        timestamp = datetime.now(timezone.utc).isoformat()
        geojson_file.updatetime = timestamp
        if args['notifyTime']:
            geojson_file.notifytime = args['notifyTime']
        if args['logger']:
            geojson_file.logger = args['logger']
        if args['size']:
            geojson_file.size = args['size']
        if args['soundings']:
            geojson_file.soundings = args['soundings']
        if args['status']:
            geojson_file.status = args['status']
        db.session.commit()
        return geojson_file

    def delete(self, fileid):
        geojson_file = GeoJSONDataModel.query.filter_by(fileid=fileid).first()
        if not geojson_file:
            abort(ReturnCodes.FILE_NOT_FOUND.value, description='That GeoJSON file does not exist in the database, and therefore cannot be deleted.')
        db.session.delete(geojson_file)
        db.session.commit()
        return ReturnCodes.RECORD_DELETED.value

api = Api(app)
api.add_resource(WIBLData, '/wibl/<string:fileid>')
api.add_resource(GeoJSONData, '/geojson/<string:fileid>')

def main() -> NoReturn:
    app.run(debug=True)

if __name__ == "__main__":
    main()
