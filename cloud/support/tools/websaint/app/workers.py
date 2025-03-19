import psycopg2
import datetime
from flask import render_template
from app.restapi import getbillingid, getbalance, getmoney, getmanager, getsupporttype, makeget
from app.grpcapi import get_subject
from app.config import Config
from app import log

logger = log.getLogger(__name__)

SERVICES_ACTIVE = [
    "compute",
    "virtual-private-cloud",
    "managed-database",
    "object-storage",
    "functions",
    "triggers",
    "kubernetes",
    "container-registry",
    "instance-group",
    "monitoring",
    "internet-of-things",
    "load-balancer",
    "ydb",
    "dns"]

def get_cloud_age(cloud_id, iam_token):
    endpoint = f'https://resource-manager.api.cloud.yandex.net/resource-manager/v1/clouds/{cloud_id}'
    data = makeget(endpoint, cloud_id, iam_token, public=True, riseerror=True, header=True)
    created_at = datetime.datetime.fromisoformat(data.json()['createdAt'][:-1])
    age = datetime.datetime.today() - created_at
    return age.days


def get_badlist():
    with open('static/bad.txt', 'r') as f:
        bad = f.read().splitlines()
    return bad


def return_template(cloud_id, iam_token):
    services = SERVICES_ACTIVE
    log.debug(services)
    grpcquota = {}

    for service in services:
        if service == 'compute-old':
            # ToDo add smtp quota
            continue
        try:
            servicequotas = get_subject(service, cloud_id, iam_token)
            grpcquota[service] = {}
            for key in servicequotas['metrics']:
                grpcquota[service][key['name']] = \
                    {'limit': key['limit'],
                     'price': Config.quotasprice.get(key['name'], 0)}
        except Exception as error:
            log.debug(error)

    bill_data = getbillingid(anyID=cloud_id, iamToken=iam_token, rise=True)
    cloud_age = get_cloud_age(cloud_id, iam_token)
    bill_data['cloud_age'] = cloud_age
    log.info(str(bill_data))
    money = getmoney(bill_data['id'], iam_token)
    manager = getmanager(bill_data['id'], iamtoken=iam_token)
    balance = getbalance(bill_data, iam_token)
    supporttype = getsupporttype(bill_data['id'], iamtoken=iam_token)
    badlist = get_badlist()
    data = render_template('price_assessors.html',
                           manager=manager,
                           services=grpcquota,
                           money=money,
                           balance=balance,
                           userinfo=bill_data,
                           cloudid=cloud_id,
                           supporttype=supporttype,
                           badlist=badlist,
                           bytequota=Config.bytequota)
    return data


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


def get_billing_data(cloud_id: str, iam_token):
    bill_data = getbillingid(anyID=cloud_id, iamToken=iam_token, rise=True)
    cloud_age = get_cloud_age(cloud_id, iam_token)
    bill_data['cloud_age'] = cloud_age
    money = getmoney(bill_data['id'], iam_token)
    manager = getmanager(bill_data['id'], iamtoken=iam_token)
    balance = getbalance(bill_data, iam_token)
    return {'money': money, 'manager': manager, 'balance': balance, 'userinfo': bill_data, 'badlist': get_badlist()}

