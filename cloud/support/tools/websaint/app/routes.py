# ToDo replace all operations away
import sqlite3
import time

import datetime

from flask import request as flskrequest
from flask import abort, render_template, make_response
from app import app
from app.restapi import getcloudid, makeget, getbillingid, shotstore, dbrequest
from app.workers import return_template, get_billing_data
from app.config import Config
from app.grpcapi import update_from_token
from app import log
from app.saint.helpers import *
from app.saint.websaint import WebSaint
from app.quotaservice import QuotaOperations
from app.quotaservice.helpers import *
from google.protobuf.json_format import MessageToJson
from .grpc_auth import AuthService


token_service = AuthService()
iam_token = token_service.get_iam_token()

# ToDo replace endpoint to use from Constants or Config
quotaservice = QuotaOperations(sa_id="", endpoint='qm.private-api.cloud.yandex.net:4224')

# ToDo replace read rei.conf -> remove usage rei.conf
LOG_DB = 'log.db'
home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('./rei.cfg'.format(home_dir))
profiles = load_profiles_from_config(config)


@app.route('/')
def index():
    return app.send_static_file('form.html')


@app.route('/sla')
def sla():
    return app.send_static_file('sla.html')


@app.route('/error')
def error(code, msg):
    log.critical(msg)
    abort(code, msg)


@app.route('/billing', methods=['GET', 'POST'])
def getbilling():
    iam_token = token_service.get_token()
    type_id = flskrequest.headers.get('X-Type')
    any_id = flskrequest.headers.get('X-AnyID')
    cloud_id = getcloudid(type_id, any_id, iam_token, rise=True)
    return return_template(cloud_id, token_service.get_iam_token())


@app.route('/cloud/<cloudid>', methods=['GET'])
@app.route('/cloud_old/<cloudid>', methods=['GET'])
def cloud(cloudid=None):
    cloud_id = getcloudid('', cloudid, iam_token, rise=True)
    return return_template(cloud_id, token_service.get_iam_token())


@app.route('/resolv', methods=['GET'])
def resolv():
    return app.send_static_file('resolvform.html')


@app.route('/regexp', methods=['GET'])
def regexpform():
    return app.send_static_file('regexp.html')


@app.route('/resolv/rawdata', methods=['POST'])
def getrawdata():
    iam_token = token_service.get_token()
    type_id = flskrequest.headers.get('X-Type')
    reqid = flskrequest.json['data']
    if type_id == 'billing_id':
        balance = makeget(Config.BILL_URL, reqid, header=True, riseerror=True, iamtoken=iam_token).json()
        id = reqid
    else:
        id = getcloudid(type_id, reqid, iam_token)
        if id is None:
            return "None", 404
        data = getbillingid(anyID=id, iamToken=iam_token)
        if data is None:
            return id, 404
        balance = makeget(Config.BILL_URL, data['id'], header=True, riseerror=True, iamtoken=iam_token).json()
    if balance['usageStatus'] == 'trial':
        paid = False
    else:
        paid = True
    if balance['state'] != 'suspended':
        active = True
    else:
        active = False

    if balance['displayStatus'] == 'SERVICE':
        return json.dumps({id: {'name': 'SERVICE',
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


@app.route('/setquota', methods=['POST'])
def setquotas():
    iam_token = token_service.get_token()
    oauth = flskrequest.headers.get('X-Oauth')
    ticket = flskrequest.headers.get('X-Ticket')
    token = flskrequest.headers.get('X-Token')
    cloud = flskrequest.headers.get('X-Cloud')

    data = datetime.datetime.now()
    dbstring = f"INSERT INTO log_approve (token, cloudid, date) VALUES ('{token}', '{cloud}', '{data}')"
    try:
        dbrequest(dbstring)
    except:
        abort(500, 'DB error')
    responce = update_from_token(token=token, iam_token=iam_token)
    log.info(f'Set quotas.\nStatus: {responce}\nToken: {token}\nTicket: {ticket}\nOauth: {oauth}')
    return responce


def query_db(query):
    with sqlite3.connect(LOG_DB) as con:
        cur = con.cursor()
        cur.execute(query)
        result = cur.fetchone()
    return result if result else None


def saint(command='', argument='', subcommand='', sub_argument=''):
    if flskrequest.form.get('command'):
        command = flskrequest.form.get('command')

    if flskrequest.form.get('command'):
        command = flskrequest.form.get('command')
        args = {
            'command': flskrequest.form.get('command'),
            'argument': flskrequest.form.get('argument'),
            'subcommand': flskrequest.form.get('subcommand'),
            'subargument': flskrequest.form.get('sub_argument')
        }
    else:
        args = {
            'command': command,
            'argument': argument,
            'subcommand': subcommand,
            'subargument': sub_argument
        }
    websaint = WebSaint()
    if command == 'eml':
        return render_template('eml.html', eml=argument)
    if command == 'c':       #+                           # done done
        return render_template('saint.html', cloud=websaint.get_cloud_info(args))
    elif command == 'f':       #+                         # done done
        return render_template('saint.html', folder=websaint.get_folder_info(args))
    elif command == 'i':     #+                           # done done
        return render_template('saint.html', instance=websaint.get_instance_info(args))
    elif command == 'ip':      #+                         # done done
        return render_template('saint.html', ip=websaint.get_ip_info(args))
    elif command == 'd':     #+                           # done done
        return render_template('saint.html', disk=websaint.get_disk_info(args))
    elif command == 'img':   #+                           # done done
        return render_template('saint.html', img=websaint.get_disk_image_info(args))
    elif command == 's3':        #+                       # done done     /acc/varkasha@yandex-team.ru/org/yc.organization-manager.yandex
        return render_template('saint.html', bucket=websaint.get_bucket(args))
    elif command == 'acc':          #+
        return render_template('saint.html', acc=websaint.get_account_info(args))
    elif command == 'simg':         # +                     # done done
        return render_template('saint.html', std_images=websaint.list_disk_standard_images(args))  # done # done
    elif command == 'gmk8s':
        return render_template('saint.html', k8s=websaint.list_k8s_cluster_masters(args)) # done # done
    elif command == 'ai':  #+
        return render_template('saint.html', ai=websaint.list_all_instances(args))        # done # done
    elif command == 'sn':  #+
        return render_template('saint.html', sn=websaint.list_all_subnets(args))          # done # done
    elif command == 'ips': #+
        return render_template('saint.html', ips=websaint.list_all_ips(args))              # done # done
    # ToDO replace rate limiter to nginx
    elif command == 'b':  #+
        ip = flskrequest.remote_addr
        ts = int(time.time())
        record = query_db(f'select timestamp, counter from req_log where ip like "{ip}"')
        if record:
            timedelta = int(time.time() - record[0])
            cnt = record[1]
            if datetime.datetime.now().hour == 23:
                query_db(f'delete from req_log where 1')
            if timedelta <= 60:
                if cnt >= 5:
                    return render_template('trylater.html')
                else:
                    query_db(f'update req_log set counter = counter + 1 where ip like "{ip}"')
            else:
                query_db(f'update req_log set counter = 0, timestamp = {ts} where ip like "{ip}"')
        else:
            log.info(f'insert {ip} into db at {ts}')
            query_db(f'insert into req_log (ip, timestamp, counter) values ("{ip}", {ts}, 1)')

        return render_template('saint.html', ba=websaint.get_billing_account(args))       # done # done
    elif command == 'gr':  #+
        return render_template('saint.html', grants=websaint.get_grants(args))                # done # done
    elif command == 'pa':  #+
        return render_template('saint.html', payments=websaint.get_payment_history(args))       # done # done
    else:
        return app.send_static_file('sainthelp.html')


app.add_url_rule('/saint/<command>/<argument>/<subcommand>/<sub_argument>', 'saint', saint, methods=['GET', 'POST'])
app.add_url_rule('/saint/<command>/<argument>/<subcommand>', 'saint', saint, methods=['GET', 'POST'])
app.add_url_rule('/saint/<command>/<argument>', 'saint', saint, methods=['GET', 'POST'])
app.add_url_rule('/saint/<command>', 'saint', saint, methods=['GET', 'POST'])
app.add_url_rule('/saint', 'saint', saint, methods=['GET', 'POST'])


@app.route('/qm/<cloud_id>', methods=['GET'])
def qm_short(cloud_id=None):
    response = quotaservice.list_requests(cloud_id, token=token_service.get_token())
    if type(response) is str():
        return make_response(response, 500)
    response = list_cloud_requests_to_json(response)
    billing_data = get_billing_data(cloud_id=cloud_id, iam_token=token_service.get_token())
    if response:
        return render_template('quotarequests.html',
                               requests=response,
                               view=[],
                               balance=billing_data['balance'],
                               money=billing_data['money'],
                               userinfo=billing_data['userinfo'],
                               badlist=billing_data['badlist']
                               )
    else:
        return make_response("Quotas request's list is empty", 404)


@app.route('/qm', methods=['GET'])
def quota_main():
    cloud_id = flskrequest.args.get('cloud_id', '')
    return render_template('quotabase.html', cloud_id=cloud_id)


@app.route('/req/<req>', methods=['GET', 'POST'])
def qm_request(req=None):
    iam_token = token_service.get_token()
    if flskrequest.method == 'GET':
        response = quotaservice.get_request(req, token=iam_token)
        response = get_quota_request_to_json(response)
        cloud_id = response[0]['cloud_id']
        billing_data = get_billing_data(cloud_id=cloud_id, iam_token=iam_token)
        return render_template('quotarequests.html',
                               requests=response,
                               view=[],
                               balance=billing_data['balance'],
                               money=billing_data['money'],
                               userinfo=billing_data['userinfo'],
                               badlist=billing_data['badlist']
                               )
    elif flskrequest.method == 'POST':
        data = json_to_quota_request(flskrequest.json)
        response = quotaservice.update_request(request_id=req,
                                               limits=data,
                                               token=token_service.get_token())
        if type(response) is str:
            return make_response(response, 404)
        else:
            return make_response(response.id, 200)


@app.route('/op/<op_id>', methods=['GET'])
def qm_operation(op_id=None):
    response = quotaservice.get_operation_request(operation_id=op_id, token=token_service.get_token())
    response = json.loads(MessageToJson(response))
    done = response.get('done', '')

    while done == '':
        response = quotaservice.get_operation_request(operation_id=op_id, token=token_service.get_token())
        response = json.loads(MessageToJson(response))
        done = response.get('done', '')
        time.sleep(0.2)
    return make_response(str(done), 200)
