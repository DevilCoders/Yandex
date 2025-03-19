#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

from flask import Flask
from flask_bootstrap import Bootstrap
from flask_restful import Api  # noqa: F401
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument(
    '-t',
    dest='template_dir',
    default=Path.cwd() / 'app' / 'templates',
    help='Absolete path to Flask templates directory'
)
parser.add_argument(
    '-u',
    dest='user',
    default='netinfra-rw',
    help='username to connect to devices via ssh'
)
parser.add_argument(
    '-k',
    dest='key',
    default='/home/netinfra-rw/.ssh/id_rsa',
    help='Absolete path to the key of ssh user'
)
args = parser.parse_args()

app = Flask(__name__, template_folder=args.template_dir)
app.config['CSRF_ENABLED'] = True
app.config['SECRET_KEY'] = 'you-will-never-guess'
bootstrap = Bootstrap(app)

from app import views  # noqa: F401
from app import api  # noqa: F401
