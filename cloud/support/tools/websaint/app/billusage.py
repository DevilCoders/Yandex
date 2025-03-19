#!/usr/bin/env python3

import json
import os
import time
import configparser
import logging as log
import collections
import random
import string
import psycopg2
import requests
from requests.adapters import HTTPAdapter
import re
from datetime import datetime
from flask import Flask, request, render_template, abort, Response


config = configparser.ConfigParser()
config.read('config.ini')

app_conf = config['APP']
db_conf = config['DB']
flask_conf = config['Flask']

OAUTH = app_conf['oAuth']
DEBUG = app_conf['Debug']
BILL_URL = app_conf['BILL_URLL']
GRANT_URL = app_conf['GRANT_URL']
QUOTA_COMP_URL = app_conf['QUOTA_COMP_URL']
QUOTA_MDB_URL = app_conf['QUOTA_MDB_URL']
QUOTA_MDB_URL_V2 = app_conf['QUOTA_MDB_URL_V2']
QUOTA_S3_URL = app_conf['QUOTA_S3_URL']
BILLINGID_URL = app_conf['BILLINGID_URL']
LOGIN_URL = app_conf['LOGIN_URL']
FOLDERS_URL = app_conf['FOLDERS_URL']
VM_URL = app_conf['VM_URL']
VM_IP = app_conf['VM_IP']
DB_HOSTNAME = db_conf['DB_HOSTNAME']
DB_USERNAME = db_conf['DB_USERNAME']
DB_NAME = db_conf['DB_NAME']
DB_PASSWD = db_conf['DB_PASSWD']
BILLUSAGE = app_conf['BILLUSAGE']

if app_conf['Logging']:
    log.basicConfig(filename=app_conf['LogFile'],
                    level=app_conf['LogLevel'],
                    format='%(asctime)s %(levelname)s %(message)s')

SECRET_KEY = flask_conf['SECRET_KEY']
USERNAME = flask_conf['USERNAME']
PASSWORD = flask_conf['PASSWORD']
PORT = flask_conf['PORT']
