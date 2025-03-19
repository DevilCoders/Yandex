# coding: utf-8
"""
uwsgi entry point
"""
from dbaas_internal_api import create_app

application = create_app()
