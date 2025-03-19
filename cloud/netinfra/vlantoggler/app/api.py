from flask_restful import Api, Resource, reqparse
from app import app
from app.netbox_helper import NetboxInterface
from app.vlantoggler import Toggler

api = Api(app)


class ToggleAPI(Resource):
    def __init__(self):
        self.reqparse = reqparse.RequestParser()
        self.reqparse.add_argument(
            'tor',
            type=str,
            required=True,
            help='No Tor name provided',
            location='json'
        )
        self.reqparse.add_argument(
            'intf',
            type=str,
            required=True,
            help='No Tor interface provided',
            location='json'
        )

        self.reqparse_post = self.reqparse.copy()
        self.reqparse_post.add_argument(
            'state',
            choices=('setup', 'prod'),
            required=True,
            help="'setup' or 'prod' must be provided in 'state' parameter",
            location='json'
        )
        super(ToggleAPI, self).__init__()

    def get(self):
        args = self.reqparse.parse_args()
        tor = args['tor'].replace(' ', '')
        intf = args['intf'].lower().replace(' ', '')

        netbox_reply = NetboxInterface(tor, intf).interface_data
        if netbox_reply['code'] != 200:
            return netbox_reply, netbox_reply['code']
        else:
            with Toggler(netbox_reply) as t:
                result = t.check

            return result, result['code']

    def post(self):
        args = self.reqparse_post.parse_args()
        tor = args['tor'].replace(' ', '')
        intf = args['intf'].lower().replace(' ', '')
        state = args['state'].lower().replace(' ', '')

        netbox_reply = NetboxInterface(tor, intf, state).interface_data
        if netbox_reply['code'] != 200:
            return netbox_reply, netbox_reply['code']
        else:
            netbox_reply['state'] = state

            with Toggler(netbox_reply) as t:
                result = t.toggle

            return result, result['code']


api.add_resource(ToggleAPI, '/api/v1.0/')
