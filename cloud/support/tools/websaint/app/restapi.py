import requests
import json
import time
from requests.adapters import HTTPAdapter
from flask import render_template, abort
from datetime import datetime
import collections
import random
import string
import psycopg2
from app.config import Config
from app import log

# ToDo set auth from function grpc_auth


def makeget(URL, ID, iamtoken, header=False, riseerror=False, public=False):
    reqsession = requests.Session()
    reqsession.mount(URL.format(ID=ID), HTTPAdapter(max_retries=6))
    if not public:
        header_title = 'X-YaCloud-SubjectToken'
        prefix = ''
    else:
        header_title = 'Authorization'
        prefix = 'Bearer '
    try:
        if header:
            r = reqsession.get(URL.format(ID=ID),
                               headers={f'{header_title}': f'{prefix}{iamtoken}'},
                               timeout=3)
        else:
            r = reqsession.get(URL.format(ID=ID), timeout=3)
        if r.status_code != 200 and riseerror:
            log.critical(r.text)
            abort(r.status_code, r.text)
        elif r.status_code != 200 and not riseerror:
            log.critical(r.text)
            r = None
        return r
    except (requests.exceptions.Timeout,
            requests.exceptions.ConnectTimeout,
            requests.exceptions.ReadTimeout,
            requests.exceptions.ConnectionError) as err:
        log.critical(err)
        if riseerror:
            log.critical(err)
            abort(500, err)
        else:
            return None


def getbalance(billData, iam_token):
    balance = makeget(Config.BILL_URL, billData['id'], iamtoken=iam_token, header=True, riseerror=True).json()
    grants = makeget(Config.GRANT_URL, billData['id'], iamtoken=iam_token, header=True, riseerror=True).json()
    startgrant = 0
    grantsumm = 0

    for grant in grants['monetaryGrants']:
        if grant['isDefault'] and (int(grant['endTime']) > time.time()):
            startgrant += float(grant['initialAmount'])

        elif (not grant['isDefault']) and (int(grant['endTime']) > time.time()):
            grantsumm += float(grant['initialAmount'])

    balance = {'grantsumm': str(grantsumm+startgrant),
               'startgrant': str(startgrant),
               'currency': balance['currency'],
               'balance': balance['balance'],
               'credit': balance['billingThreshold']}
    return balance


def getmoney(billingid, iam_token):
    transactions = makeget(Config.BILLMONEYLIST, billingid, iamtoken=iam_token, header=True, riseerror=True).json()
    money = 0
    for transaction in transactions['transactions']:
        if transaction['status'] == 'ok':
            money += float(transaction['amount'])
    return money


def getmanager(billid, iamtoken):
    manager = makeget(Config.MANAGED, iamtoken=iamtoken, ID=billid, header=True, riseerror=False)

    if manager:
        return True
    else:
        return False


def getsupporttype(billid, iamtoken):
    supportype = makeget(Config.SUPTYPE, iamtoken=iamtoken, ID=billid, header=True, riseerror=False)
    type = supportype.json()['current']['priorityLevel']
    return type


def getcloudid(typeID, ID, iam_token, rise=False):
    print(typeID, ID)
    if typeID == 'folder_id':
        r = makeget(Config.FOLDERS_URL, ID, riseerror=rise, iamtoken=iam_token)
        if r is None:
            return None
        elif not json.loads(r.text)['result']:
            abort(404, 'Folder not Found')
        cloud_id = json.loads(r.text)['result'][0].get('cloudId')
    elif typeID[0:4] == 'vmid':
        r = makeget(Config.VM_URL, ID, riseerror=rise, iamtoken=iam_token)
        if r is None:
            return None
        cloud_id = json.loads(r.text).get('cloud_id')
    elif typeID == 'vmip':
        r = makeget(Config.VM_IP, ID, header=True, riseerror=rise, iamtoken=iam_token)
        if r is None:
            return None
        ID = json.loads(r.text).get('instance_id')
        r = makeget(Config.VM_URL, ID, riseerror=rise, iamtoken=iam_token)
        cloud_id = json.loads(r.text).get('cloud_id')
    elif typeID == 'token':
        data = dbrequest("SELECT data from qfiles where name = '{}'".format(ID), response=True)
        cloud_id = data[0][0].pop('cloudId')
        return render_template('tokenview.html', token=data[0][0], cloudId=cloud_id, bytequota=Config.bytequota)
    elif typeID == 'balance':
        billData = getbillingid(anyID=ID, iamToken=iam_token, rise=True)
        balance = getbalance(billData, iam_token)
        return render_template('balance.html', userinfo=billData, balance=balance)
    else:
        cloud_id = ID
    return cloud_id


def getbillingid(anyID, iamToken, rise=False):
    data = '{{"{}":"{}"}}'.format('cloud_id', anyID)
    r = requests.post(Config.BILLINGID_URL,
                      headers={'X-YaCloud-SubjectToken': '{}'.format(iamToken)}, json=json.loads(data))
    if r.status_code == 200:
        return r.json()
    elif r.status_code == 404 and not rise:
        return None
    elif r.status_code == 404 and rise:
        log.critical(r.json()['message'])
        abort(r.status_code, r.json()['message'])
    return r.json()


def getuserinfo(billID, cloudId, iam_token):
    quotas_compute_raw = makeget(Config.QUOTA_COMP_URL, cloudId,
                                 iamtoken=iam_token, header=True, riseerror=True).json()
    quotas_mdb_v2 = makeget(Config.QUOTA_MDB_URL_V2, cloudId,
                            iamtoken=iam_token, header=True, riseerror=True).json()
    quotas_s3 = makeget(Config.QUOTA_S3_URL, cloudId, header=True,
                        iamtoken=iam_token, riseerror=True).json()

    balance = getbalance(billID)
    quotas_compute = {}

    for quota in quotas_compute_raw["metrics"]:
        quotas_compute[quota['name']] = {'value': quota['value'],
                                         'limit': quota['limit'],
                                         'price': Config.quotasprice.get(quota['name'], 0)}
    quotas_mdb = {}
    mdb_metrics = quotas_mdb_v2['metrics']
    for quota in mdb_metrics:
        name = quota['name']
        quotas_mdb[name] = {'limit': quota['limit'],
                            'value': quota['usage'],
                            'price': Config.quotasprice.get(name, 0)}

    quotas_compute = collections.OrderedDict(sorted(quotas_compute.items()))
    quotas_mdb = collections.OrderedDict(sorted(quotas_mdb.items()))
    return balance, quotas_compute, quotas_mdb, quotas_s3


def shotstore(data):
    flag = True
    token = ''.join([random.choice(string.ascii_letters) for x in range(6)])
    names = dbrequest("select name from qfiles", response=True)
    namelist = [name[0] for name in names]
    while flag:
        if token in namelist:
            token = ''.join([random.choice(string.ascii_letters) for x in range(6)])
        else:
            break
    dbrequest("INSERT INTO qfiles (name, data, time) VALUES ('{}','{}', '{}')"
              .format(token, json.dumps(data), datetime.now()))
    return token


def dbrequest(reqstring, response=False):
    conn = psycopg2.connect("""
        host={hostname}
        port=6432
        dbname={dbname}
        user={dbuser}
        password={dbpasswd}
        target_session_attrs=read-write
        sslmode=verify-full
    """.format(hostname=Config.DB_HOSTNAME,
               dbname=Config.DB_NAME,
               dbuser=Config.DB_USERNAME,
               dbpasswd=Config.DB_PASSWD))

    dbcursor = conn.cursor()
    dbcursor.execute(reqstring)
    if response:
        data = dbcursor.fetchall()
        conn.close()
        return data
    conn.commit()
    conn.close()


def get_cloud_age(cloud_id, iam_token):
    endpoint = 'https://resource-manager.api.cloud.yandex.net/resource-manager/v1/clouds/'
    print(makeget(endpoint, cloud_id, iam_token, public=True))
