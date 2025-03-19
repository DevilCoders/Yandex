# coding: utf-8
"""
UWSGI entry point for the MDB IDM service.
"""
from idm_service import create_app

application = create_app()

if __name__ == '__main__':
    application.run(host='0.0.0.0', debug=True)  # nosec
