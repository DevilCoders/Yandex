import os
import configparser

config = configparser.ConfigParser()
config.read('config.ini')

class Config(object):
    app_conf = config['APP']
    db_conf = config['DB']
    flask_conf = config['Flask']

    PORT = flask_conf['PORT']
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
    IAMTOKEN = {}
    quotasprice = {'cores': '0.7017',
                   'instance-cores': '0.7017',
                   'memory': '0.24',
                   'network-hdd-total-disk-size': '0.0101666666666667',
                   'network-ssd-total-disk-size': '0.0101666666666667',
                   'total-disk-size': '0.0101666666666667',
                   'total-snapshot-size': '0.00304166666666667',
                   'gpus': '155.9530',
                   'mdb.hdd.size': '0.003177917',
                   'mdb.ssd.size': '0.011299444',
                   'mdb.gpu.count': '0',
                   'mdb.cpu.count': '1.23',
                   'mdb.memory.size': '0.35'
                   }

    bytequota = ['memory',
                 'network-hdd-total-disk-size',
                 'network-ssd-total-disk-size',
                 'total-disk-size',
                 'total-snapshot-size',
                 'mdb.ssd.size',
                 'mdb.hdd.size',
                 'mdb.memory.size',
                 'total_size_quota']
