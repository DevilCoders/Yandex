from flask import request as flskrequest
from app import app
from app.restapi import *
from app.config import *


@app.route('/')
def index():
    return app.send_static_file('form.html')


@app.route('/sla')
def sla():
    return app.send_static_file('sla.html')


@app.route('/error')
def error(code, msg):
    app.log.critical(msg)
    abort(code, msg)


@app.route('/billing', methods=['GET', 'POST'])
def getbilling():
    iam_token = getiamtoken(Config.OAUTH)
    type_id = flskrequest.headers.get('X-Type')
    any_id = flskrequest.headers.get('X-AnyID')
    cloud_id = getcloudid(type_id, any_id, iam_token, rise=True)
    return return_template(cloud_id, iam_token)


@app.route('/cloud/<cloudid>', methods=['GET'])
def cloud(cloudid=None):
    iam_token = getiamtoken(Config.OAUTH)
    id = getcloudid('', cloudid, iam_token, rise=True)
    return return_template(id, iam_token)


@app.route('/resolv', methods=['GET'])
def resolv():
    return app.send_static_file('resolvform.html')


@app.route('/regexp', methods=['GET'])
def regexpform():
    return app.send_static_file('regexp.html')


# @app.route('/get/<id>', methods=['GET'])
# def getdata(id=None):
#     return resolvid(id)


@app.route('/resolv/rawdata', methods=['POST'])
def getrawdata():
    iam_token = getiamtoken(Config.OAUTH)
    type_id = flskrequest.headers.get('X-Type')
    reqid = flskrequest.json['data']
    # params = request.json['params']
    # params: {
    #  individual: bool,
    #  company: bool,
    #  service: bool,
    #  aname: bool,
    #  usestatus: bool}
    if type_id == 'billing_id':
        balance = makeget(Config.BILL_URL, reqid, header=True, riseerror=True).json()
        id = reqid
    else:
        id = getcloudid(type_id, reqid, iam_token)
        if id == None: return "None", 404
        data = getbillingid(anyID=id, iamToken=iam_token)
        if data == None: return id, 404
        balance = makeget(Config.BILL_URL, data['id'], header=True, riseerror=True).json()
    if balance['usageStatus'] == 'trial':
        paid = False
    else:
        paid = True
    if balance['state'] != 'suspended':
        active = True
    else:
        active = False
    #{cloudid: {'paid': bool, 'active': bool, 'name': str}}
    if balance['displayStatus'] == 'SERVICE':
        return json.dumps({id:{'name': 'SERVICE',
                               'id': reqid}}), 200
    if balance['personType'] == 'company':
        return json.dumps({id: {'paid': paid,
                                     'active': active,
                                     'name': balance['person']['company']['name'],
                                     'id': reqid}}), 200
    if balance['personType'] == 'individual':
        return json.dumps({id: {'paid': paid,
                                     'active': active,
                                     'name': 'individual',
                                     'id': reqid}}), 200
    return None


@app.route('/short', methods=['POST'])
def short():
    data = flskrequest.json
    for key in data.keys():
        if key != 'cloudId':
            for quota in data[key].keys():
                if quota in Config.bytequota:
                    data[key][quota] = data[key][quota] * 1073741824

    return json.dumps({'link': shotstore(data)})


@app.route('/qfiles/<token>', methods=['GET'])
def qfiles(token=None):
    data = dbrequest("SELECT data from qfiles where name = '{}'".format(token), response=True)
    return json.dumps(data[0][0])
