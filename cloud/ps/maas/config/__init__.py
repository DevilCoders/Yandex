import configparser
import logging
import os
import uuid
import requests
import json

logger = logging.getLogger('ConfigLoaderLogger')
logger.setLevel(logging.DEBUG)
f = logging.FileHandler('main.log')
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
f.setFormatter(formatter)
logger.addHandler(f)

logger.debug('Initializing config parser')
settings = configparser.ConfigParser()
logger.debug('Trying to read config file')

if not os.path.isfile('config.ini'):
    logger.error('Config file not found')
    exit(1)
settings.read('config.ini')
logger.debug('Config file exists. Parsing data')

#API CA_BUNDLE
logger.debug('Trying to load CA bundle for private API')
if not os.path.isfile('allCAs.pem'):
    logger.error('CA bundle is missing. Shutting down')
    exit(1)
os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'
logger.debug('PEM bundle found.')


#API urls config
FOLDERS_URL = settings.get('API_URLS', 'folders_url')
INSTANCES_URL = settings.get('API_URLS', 'instances_url')

#SOLOMON urls config
GRAPH_URL = settings.get('SOLOMON_URLS', 'graph')

#SOLOMON auth settings
SOLOMON_TOKEN = settings.get('SOLOMON_AUTH', 'token')

#API auth settings
uid = str(uuid.uuid4())
logger.debug('Trying to get IAM token. Request: ' + uid)
Header = {'Content-Type':'application/json', 'X-Request-ID': uid}
IAM_URL = settings.get('API_AUTH', 'iam')
oauth = settings.get('API_AUTH', 'oauth')
try:
    res = requests.post(IAM_URL, json = {'oauthToken': oauth}, headers = Header, timeout=10)
except Exception as e:
    logger.error('Unable to request IAM token: ' + str(e))
if res.status_code != 200:
    logger.error('API returned: ' + str(res.status_code) + ' Answer: ' + res.text)
iam = json.loads(res.text)
API_TOKEN = iam['iamToken']