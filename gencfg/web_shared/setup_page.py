import service_ctl

from flask_restful import Resource, reqparse
from exit_msg import ExitCodes
from config import *

import os

# when this file exists we serve only /setup pages
# therefore user cannot change data
SHUTDOWN_FILE = os.path.join(os.path.dirname(__file__), '.shutdown')
shutdown_mode = os.path.exists(SHUTDOWN_FILE)


class PageSetup(Resource):
    @classmethod
    def init(cls, api):
        cls.api = api
        cls.logger = api.app.logger
        return cls

    # TODO: after building good deployment system we need to change these permissions!!!
    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def gencfg_authorize_post(self, login, request):
        del login, request
        return True

    def get(self):
        global shutdown_mode
        return {
            "pid": os.getpid(),
            "uuid": self.api.uuid,
            "status": "started" if not shutdown_mode else "stopped",
            "actions": {
                "start": dict(enabled=shutdown_mode),
                "stop": dict(enabled=not shutdown_mode),
                "restart": dict(enabled=True),
                "die": dict(enabled=True),
                "update_all": dict(enabled=True),
            }
        }

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument("action", type=str, help="Action to perform")
        args = parser.parse_args()

        if args.action not in self.get()['actions']:
            raise Exception('Unsupported action value "%s"' % args.action)

        func = getattr(self, args.action)
        return func()

    def start(self):
        global shutdown_mode
        if not shutdown_mode:
            return {'reply': 'Server is already started'}  # no finalize intentionally

        if os.path.exists(SHUTDOWN_FILE):
            os.unlink(SHUTDOWN_FILE)
        self.shutdown_server()  # assume that we run in loop script
        return {'reply': 'Server will be restarted now in normal mode'}  # no finalize intentionally

    def stop(self):
        global shutdown_mode
        if shutdown_mode:
            return {'reply': 'Server is already in shutdown mode'}  # no finalize intentionally

        self.shutdown_server()  # assume that we run in loop script
        if not os.path.exists(SHUTDOWN_FILE):
            f = open(SHUTDOWN_FILE, 'w')
            f.close()
        message = 'Server will be restarted now in shutdown mode (only /setup page available)'
        return {'reply': message}  # no finalize intentionally

    def restart(self):
        global shutdown_mode
        mode = 'shutdown' if shutdown_mode else 'normal'
        self.shutdown_server()  # assume that we run in loop script
        message = 'Server will be restarted now in current (%s) mode.' % mode
        return {'reply': message}  # no finalize intentionally

    def die(self):
        self.shutdown_server(ExitCodes.DIE)
        message = 'Server will be killed.'
        return {'reply': message}  # no finalize intentionally

    def update_all(self):
        self.shutdown_server(ExitCodes.UPDATE_ALL)
        message = 'Server will be restarted with a new code base.'
        return {'reply': message}  # no finalize intentionally

    def shutdown_server(self, exit_code=ExitCodes.NORMAL):
        service_ctl.shutdown(exit_code)
