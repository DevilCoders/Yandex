import json
from app import Config
from app.helpers.Tracker import Tracker
import grpc
import datetime
from app import log

tracker = Tracker(useragent='support_qm',
                  base_url='https://st-api.yandex-team.ru/v2',
                  st_oauth=Config.TRACKER_TOKEN)


def get_request_status(num: int):
    status = ['STATUS_UNSPECIFIED',
              'PENDING',
              'PROCESSING',
              'PROCESSED',
              'CANCELED',
              'DELETING']
    return status[num]


def get_limit_status(num: int):
    status = ['STATUS_UNSPECIFIED',
              'PENDING',
              'PROCESSING',
              'PARTIAL_APPROVED',
              'APPROVED',
              'REJECTED',
              'CANCELED']
    return status[num]


def to_json(data: dict):
    return json.dumps(data)


def list_cloud_requests_to_json(data):
    responce = []
    # ToDo if type str send error
    try:
        for req in data.quota_requests:
            responce.append(get_quota_request_to_json(req)[0])
    except AttributeError as error:
        log.error(error)
    return responce


def get_quota_request_to_json(data):
    ticket_id = tracker.get_ticket_by_issue_id(data.issue_id)
    try:
        ticket_id = ticket_id[0]
    except TypeError as error:
        log.error(error)
        ticket_id = ''
    st_link = f'https://st.yandex-team.ru/{ticket_id}'
    response = {'cloud_id': data.cloud_id,
                'status': get_request_status(data.status),
                'issue_id': data.issue_id,
                'req_link': st_link,
                'req_id': data.id,
                'quota_limits': []}

    for limit in data.quota_limits:
        limits = {'quota_id': limit.quota_id,
                  'status': get_limit_status(limit.status)}
        if str(limit.quota_id).split('.')[-1] == 'size':
            human_desired_limit = bytes_to_human(limit.desired_limit)
            human_approved_limit = bytes_to_human(limit.approved_limit)
            limits['desired_limit'] = human_desired_limit
            limits['approved_limit'] = human_approved_limit
        else:
            limits['desired_limit'] = limit.desired_limit
            limits['approved_limit'] = limit.approved_limit
        response['quota_limits'].append(limits)

    return [response, ]


def json_to_quota_request(data: dict):
    update_actions = []
    for key in data.keys():
        limit = {}
        if key.split('.')[-1] == 'size':
            approved_limit = human_to_bytes(float(data[key]["approved_limit"]))
            desired_limit = human_to_bytes(float(data[key]["desired_limit"]))
        else:
            approved_limit = data[key]["approved_limit"]
            desired_limit = data[key]["desired_limit"]
        print(approved_limit, desired_limit)
        if data[key]['approved']:
            limit['approved_quota_limit'] = {"quota_id": key,
                                             "approved_limit": float(approved_limit)}
        else:
            limit['rejected_quota_limit'] = {"quota_id": key,
                                             "message": f"request cannot be approved with: {desired_limit}"}
        update_actions.append(limit)
    return update_actions


def bytes_to_human(data: int):
    return data / (1024 * 1024 * 1024)


def human_to_bytes(data: int):
    print(data)
    return data * (1024 * 1024 * 1024)


def get_grpc_channel(service_stub, endpoint, iam_token=""):
    """
    get encrypted grpc channel from stub, token and endpoint
    :param service_stub: grpc service stub
    :param endpoint: grpc endpoint string
    :param iam_token: string
    :return:
    """
    ssl = './allCA.crt'
    with open(ssl, 'rb') as cert:
        ssl_creds = grpc.ssl_channel_credentials(cert.read())
    call_creds = grpc.access_token_call_credentials(iam_token)
    chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

    stub = service_stub  # i.e.: folder_service.FolderServiceStub
    channel = grpc.secure_channel(endpoint, chan_creds)

    return stub(channel)


def timestamp_resolve(timestamp):
    return datetime.fromtimestamp(timestamp).isoformat()
