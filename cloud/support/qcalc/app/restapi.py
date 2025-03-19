import requests
import os
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


def makeget(URL, ID, header=False, riseerror=False):
    iamtoken = getiamtoken(Config.OAUTH)
    # os.environ['REQUESTS_CA_BUNDLE'] = 'static/allCAs.pem'
    reqsession = requests.Session()
    reqsession.mount(URL.format(ID=ID), HTTPAdapter(max_retries=6))
    try:
        if header:
            r = reqsession.get(URL.format(ID=ID),
                               headers={'X-YaCloud-SubjectToken': '{}'.format(iamtoken)},
                               timeout=3)
        else:
            r = reqsession.get(URL.format(ID=ID), timeout=3)
        if r.status_code != 200 and riseerror:
            pass
            # log.critical(r.text)
            # error(r.status_code, r.text)
        elif r.status_code != 200 and not riseerror:
            # log.critical(r.text)
            r = None
        return r
    except (requests.exceptions.Timeout,
            requests.exceptions.ConnectTimeout,
            requests.exceptions.ReadTimeout,
            requests.exceptions.ConnectionError) as err:
        # log.critical(err)
        if riseerror:
            pass
            # error(500, err)
        else:
            return None


def getbalance(billData):
    balance = makeget(Config.BILL_URL, billData['id'], header=True, riseerror=True).json()
    grants = makeget(Config.GRANT_URL, billData['id'], header=True, riseerror=True).json()
    grantsumm = 0
    computegrant = 0
    othergrant = 0

    for grant in grants['monetaryGrants']:
        if billData['paymentType'] == 'card' and int(grant['endTime']) > time.time():
            if grant['initialAmount'] == '1000' and grant['isDefault']:
                computegrant += float(grant['initialAmount']) - float(grant['consumedAmount'])
                continue
            elif grant['initialAmount'] == '3000' and grant['isDefault']:
                othergrant += float(grant['initialAmount']) - float(grant['consumedAmount'])
                continue
        if int(grant['endTime']) > time.time():
                grantsumm += float(grant['initialAmount']) - float(grant['consumedAmount'])

    grantsumm += othergrant + computegrant

    balance = {'credit': balance['billingThreshold'],
               'balance': balance['balance'],
               'grantsumm': str(grantsumm),
               'grants':{'compute':computegrant,'other':othergrant},
               'currency': balance['currency']}

    return balance


def getcloudid(typeID, ID, iam_token, rise=False):
    if typeID == 'folder_id':
        r = makeget(Config.FOLDERS_URL, ID, riseerror=rise)
        if r is None:
            return None
        elif not json.loads(r.text)['result']:
            abort(404, 'Folder not Found')
        cloud_id = json.loads(r.text)['result'][0].get('cloudId')
    elif typeID[0:4] == 'vmid':
        r = makeget(Config.VM_URL, ID, riseerror=rise)
        if r is None:
            return None
        cloud_id = json.loads(r.text).get('cloud_id')
    elif typeID == 'vmip':
        r = makeget(Config.VM_IP, ID, header=True, riseerror=rise)
        if r is None:
            return None
        ID = json.loads(r.text).get('instance_id')
        r = makeget(Config.VM_URL, ID, riseerror=rise)
        cloud_id = json.loads(r.text).get('cloud_id')
    elif typeID == 'token':
        data = dbrequest("SELECT data from qfiles where name = '{}'".format(ID), response=True)
        cloud_id = data[0][0].pop('cloudId')
        return render_template('tokenview.html', token=data[0][0], cloudId=cloud_id, bytequota=Config.bytequota)
    elif typeID == 'balance':
        billData = getbillingid(anyID=ID, iamToken=iam_token, rise=True)
        balance = getbalance(billData)
        return render_template('balance.html', userinfo=billData,balance=balance)
    else:
        cloud_id = ID
    return cloud_id


def return_template(cloud_id, iam_token):
    bill_data = getbillingid(anyID=cloud_id, iamToken=iam_token, rise=True)
    # getusage(cloud_id, billData)
    balance, quotas_compute, quotas_mdb, quotas_s3 = getuserinfo(bill_data, cloud_id)
    data = render_template('price.html',
                           balance=balance,
                           quotas=quotas_compute,
                           quotas_mdb=quotas_mdb,
                           quotas_s3=quotas_s3,
                           userinfo=bill_data,
                           cloudid=cloud_id)
    return data


def getiamtoken(oauthToken):
    global IAMTOKEN
    try:
        iamtoken = IAMTOKEN['token']
        if IAMTOKEN['expiretime'] < datetime.now():
            iamtoken = posttoiam(oauthToken)
            return iamtoken
        else:
            return iamtoken
    except:
        # log.info('Update IAM-token')
        iamtoken = posttoiam(oauthToken)
        return iamtoken


def posttoiam(oauthToken):
    global IAMTOKEN
    # os.environ['REQUESTS_CA_BUNDLE'] = 'static/allCAs.pem'
    IAM_URL = 'http://identity.private-api.cloud.yandex.net:4336/v1/tokens'
    r = requests.post(IAM_URL, json={"oauthToken": oauthToken})
    if r.status_code != 200:
        msg = 'Something wrong happened during acquiring IAM token. API Response: {} '.format(r.status_code)
        # error(r.status_code, msg)
    data = json.loads(r.text)
    expiretime = datetime.strptime(data['expiresAt'].split('+')[0], '%Y-%m-%dT%H:%M:%S.%f')
    iamtoken = data['iamToken']
    Config.IAMTOKEN['token'] = iamtoken
    Config.IAMTOKEN['expiretime'] = expiretime
    return Config.IAMTOKEN['token']


def getbillingid(anyID, iamToken, rise=False):
    data = '{{"{}":"{}"}}'.format('cloud_id', anyID)
    # os.environ['REQUESTS_CA_BUNDLE'] = 'static/allCAs.pem'
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


def getuserinfo(billID, cloudId):
    quotas_compute_raw = makeget(Config.QUOTA_COMP_URL, cloudId, header=True, riseerror=True).json()
    quotas_mdb_v2 = makeget(Config.QUOTA_MDB_URL_V2, cloudId, header=True, riseerror=True).json()
    quotas_s3 = makeget(Config.QUOTA_S3_URL, cloudId, header=True, riseerror=True).json()

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
    linkFileName = ''.join([random.choice(string.ascii_letters) for x in range(6)])
    dbrequest("INSERT INTO qfiles (name, data) VALUES ('{}','{}')".format(linkFileName, json.dumps(data)))
    return linkFileName


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


def getusage(billid, iamToken):
    clouds = makeget(Config.CLOUDLIST, billid['id'], header=True)
    cloudids = []
    for cloud in json.loads(clouds.text)['clouds']:
        cloudids.append(cloud['id'])
    cloudids.append("")
    skus = makeget(Config.BILLUSAGE, billid['id'], header=True)
    data = json.loads(skus.text)
    skusid = []
    for service in data['usageMeta']['skus']:
        skusid.append(service['id'])
    start = "2020-04-10"
    end = "2020-05-10"

    data = {
    "startDate": start,
    "endDate": end,
    "cloudIds": cloudids,
    "skuIds": skusid}
    os.environ['REQUESTS_CA_BUNDLE'] = 'static/allCAs.pem'
    r = requests.post(Config.USAGEDATA.format(ID=billid['id']),
                      headers={'X-YaCloud-SubjectToken': '{}'.format(iamToken['token'])}, json=data)
    print(r.json())

