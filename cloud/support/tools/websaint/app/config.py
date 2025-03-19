import os
import configparser
import json

config = configparser.ConfigParser()
config.read('config.ini')


class Config(object):

    app_conf = config['APP']
    db_conf = config['DB']
    flask_conf = config['Flask']

    PORT = int(flask_conf['PORT'])
    SECRET_KEY = os.environ.get('SECRET_KEY') or flask_conf['SECRET_KEY']
    DEBUG = flask_conf.getboolean('DEBUG')

    OAUTH = app_conf['oAuth']
    LOGGING = app_conf['Logging']
    LOGFILE = app_conf['LogFile']
    LOGLEVEL = app_conf['LogLevel']

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
    CLOUDLIST = app_conf['CLOUDLIST']
    USAGEDATA = app_conf['USAGEDATA']
    MANAGED = app_conf['MANAGED']
    BILLMONEYLIST = app_conf['BILLMONEYLIST']
    BILLMONEY = app_conf['BILLMONEY']
    SUPTYPE = app_conf['SUPTYPE']

    SSL_PATH = app_conf['SSL_PATH']
    IAMTOKEN = {}
    quotasprice = {'compute.instanceCores.count': '0.7017',
                   'compute.instanceMemory.size': '0.24',
                   'compute.hddDisks.size': '0.0101666666666667',
                   'compute.ssdDisks.size': '0.0101666666666667',
                   'compute.snapshots.size': '0.00304166666666667',
                   'compute.instanceGpus.count': '155.9530',
                   'mdb.hdd.size': '0.003177917',
                   'mdb.ssd.size': '0.011299444',
                   'mdb.gpu.count': '0',
                   'mdb.cpu.count': '1.23',
                   'mdb.memory.size': '0.35'
                   }

    bytequota = ['network-hdd-total-disk-size', 'network-ssd-total-disk-size', 'memory',
                 'total-snapshot-size', 'mdb.hdd.size', 'mdb.ssd.size', 'mdb.memory.size',
                 'storage.volume.size', 'serverless.memory.size', 'managed-kubernetes.memory.size',
                 'managed-kubernetes.disk.size', 'compute.hddDisks.size', 'compute.instanceMemory.size',
                 'compute.snapshots.size', 'compute.ssdDisks.size', 'compute.ssdNonReplicatedDisks.size',
                 'compute.ssdFilesystems.size', 'compute.hddFilesystems.size', 'ydb.dedicatedComputeMemory.size'
                 ]


    try:
        account_id = os.environ['SA_ID']
        key_id = os.environ['KEY_ID']
        private_key = os.environ['PRIVATE_KEY'].replace('\\n', '\n')
        TRACKER_TOKEN = os.environ['TRACKER_TOKEN']

    except KeyError as error:
        # ToDo setup path to keyfile from startup key or in Config
        keys = open('/home/apereshein/.config/yandex-cloud/keyacc.json', 'r').readlines()
        data = json.loads(''.join(keys))
        private_key = data.get('private_key')
        account_id = data.get('user_account_id')
        key_id = data.get('id')
        TRACKER_TOKEN = ''
