#!/usr/bin/env python3
"""
Entry point for the MDB IDM service.
"""
from idm_service import create_app

APP = create_app()


def main():
    APP.run(host='0.0.0.0', debug=True)  # nosec


if __name__ == '__main__':
    main()
