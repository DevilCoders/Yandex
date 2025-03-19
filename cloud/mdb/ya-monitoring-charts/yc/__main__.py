from __future__ import print_function

import argparse
import grpc
import os
import sys
import json
import re

from google.protobuf import text_format
from solomon.protos.monitoring.v3 import dashboard_service_pb2 as internal_dashboard_service_pb2
from solomon.protos.monitoring.v3 import dashboard_service_pb2_grpc as internal_dashboard_service_pb2_grpc
from solomon.protos.monitoring.v3.cloud import dashboard_service_pb2 as external_dashboard_service_pb2
from solomon.protos.monitoring.v3.cloud import dashboard_service_pb2_grpc as external_dashboard_service_pb2_grpc
from solomon.protos.monitoring.v3.cloud.priv import service_dashboard_service_pb2
from solomon.protos.monitoring.v3.cloud.priv import service_dashboard_service_pb2_grpc
from google.protobuf.json_format import MessageToDict, ParseDict, MessageToJson

PREPROD_ENDPOINT = {
    'host': 'monitoring.private-api.cloud-preprod.yandex.net',
    'port': '443'
}

PRODUCTION_ENDPOINT = {
    'host': 'monitoring.private-api.cloud.yandex.net',
    'port': '443'
}

PORTO_ENDPOINT = {
    'host': 'solomon.yandex.net',
    'port': '443'
}

CA_PATH = "cacert.pem"
INTERNAL_TOKEN = 'SOLOMON_OAUTH_TOKEN'
EXTERNAL_TOKEN = 'MONITORING_IAM_TOKEN'


def proto_to_text(msg):
    return text_format.MessageToString(msg)


def validate_env_var(var):
    if var not in os.environ:
        print("Error: {} env variable is not set".format(var))
        sys.exit(1)


def confirm():
    print("You are working with PROD. Did you get review these changes with Monitoring team? ", end='')
    while True:
        try:
            ch = raw_input("[y/N]: ")
        except EOFError as e:
            continue
        except KeyboardInterrupt as e:
            print('')
            sys.exit(1)

        if ch in ['y', 'Y']:
            return True
        elif ch in ['', 'n', 'N']:
            return False
        else:
            print("Invalid choice, please enter Y or N.")


def print_grpc_error(msg, e):
    print('Error {}'.format(msg))
    print('Details: ' + e.details())
    print('Error: ' + e.code().name)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=["create", "list", "copy", 'get', 'update', 'delete', 'sync'])
    parser.add_argument("-e", "--environment", required=True, choices=["prod", "preprod", "porto"])
    parser.add_argument("-e1", "--environment1", choices=["porto"])
    parser.add_argument("-d", "--dashboard-id")
    parser.add_argument("-s", "--service-dashboard-id")
    parser.add_argument("--service")
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("-c", "--config-path")

    args = parser.parse_args()

    return args


def prepare_creds(environment):
    creds = {}

    with open(CA_PATH, 'rb') as fd:
        root_cert = fd.read()
    creds["cert"] = grpc.ssl_channel_credentials(root_certificates=root_cert)

    if environment == 'porto':
        creds["metadata"] = [('authorization', 'OAuth {}'.format(os.environ[INTERNAL_TOKEN]))]
    else:
        creds["metadata"] = [('authorization', 'Bearer {}'.format(os.environ[EXTERNAL_TOKEN]))]

    return creds


def create_channel(environment, creds):
    if environment == "preprod":
        host = PREPROD_ENDPOINT["host"]
        port = PREPROD_ENDPOINT["port"]
    elif environment == "prod":
        host = PRODUCTION_ENDPOINT["host"]
        port = PRODUCTION_ENDPOINT["port"]
    elif environment == "porto":
        host = PORTO_ENDPOINT["host"]
        port = PORTO_ENDPOINT["port"]
    else:
        raise RuntimeError("Unhandled environment {}".format(environment))

    channel = grpc.secure_channel("{}:{}".format(host, port), creds["cert"])

    return channel


def create_dashboard(args, creds, channel, config):
    if args.environment in ['prod', 'preprod']:
        stub = external_dashboard_service_pb2_grpc.DashboardServiceStub(channel)
        create_request = external_dashboard_service_pb2.CreateDashboardRequest()
    else:
        stub = internal_dashboard_service_pb2_grpc.DashboardServiceStub(channel)
        create_request = internal_dashboard_service_pb2.CreateDashboardRequest()

    create_request.service = args.service
    create_request.name = config.get("name", 'empty-dashboard')

    for widget in config.get('widgets', []):
        print(widget)
        w = create_request.widgets.add()
        ParseDict(widget, w)

    ParseDict(config.get('parametrization', {}), create_request.parametrization)

    try:
        response = stub.Create(request=create_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('creating service dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    return response


def create_service_dashboard(args, creds, channel, config):
    stub = service_dashboard_service_pb2_grpc.ServiceDashboardServiceStub(channel)

    create_request = service_dashboard_service_pb2.CreateServiceDashboardRequest()
    create_request.service = args.service
    create_request.name = config.get("name", 'empty-dashboard')

    for widget in config.get('widgets', []):
        print(widget)
        w = create_request.widgets.add()
        ParseDict(widget, w)

    ParseDict(config.get('parametrization', {}), create_request.parametrization)

    try:
        response = stub.Create(request=create_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('creating service dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    return response


def list_service_dashboards(args, creds, channel):
    stub = service_dashboard_service_pb2_grpc.ServiceDashboardServiceStub(channel)

    list_request = service_dashboard_service_pb2.ListServiceDashboardRequest()
    list_request.service = args.service
    list_request.page_size = 1000

    try:
        response = stub.List(request=list_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('listing service dashboards', e)
        if args.debug:
            raise e
        print("Method not implemented yet")
        sys.exit(1)

    print(proto_to_text(response))


def get_dashboard(args, creds, channel, dashboard_id, environment):
    if environment in ['prod', 'preprod']:
        stub = external_dashboard_service_pb2_grpc.DashboardServiceStub(channel)
        get_request = external_dashboard_service_pb2.GetDashboardRequest()
    else:
        stub = internal_dashboard_service_pb2_grpc.DashboardServiceStub(channel)
        get_request = internal_dashboard_service_pb2.GetDashboardRequest()

    get_request.dashboard_id = dashboard_id

    try:
        get_response = stub.Get(request=get_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('getting dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    return get_response


def get_service_dashboard(args, creds, channel, service_dashboard_id):
    stub = service_dashboard_service_pb2_grpc.ServiceDashboardServiceStub(channel)

    get_request = service_dashboard_service_pb2.GetServiceDashboardRequest()
    get_request.service_dashboard_id = service_dashboard_id

    try:
        get_response = stub.Get(request=get_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('getting service dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    return get_response


def load_config(filename):
    with open(filename, 'r') as f:
        return json.load(f)


def print_config(config_path, response):
    with open(config_path, 'w') as f:
        print(json.dumps(response, indent=4, sort_keys=True), file=f)


def update_dashboard(args, creds, channel, config, service_dashboard=False):
    if service_dashboard:
        stub = service_dashboard_service_pb2_grpc.ServiceDashboardServiceStub(channel)

        update_request = service_dashboard_service_pb2.UpdateServiceDashboardRequest()

        update_request.service_dashboard_id = config['id']
    else:
        if args.environment in ['prod', 'preprod']:
            stub = external_dashboard_service_pb2_grpc.DashboardServiceStub(channel)
            update_request = external_dashboard_service_pb2.UpdateDashboardRequest()
        else:
            stub = internal_dashboard_service_pb2_grpc.DashboardServiceStub(channel)
            update_request = internal_dashboard_service_pb2.UpdateDashboardRequest()

        update_request.dashboard_id = config['id']

    update_request.name = config['name']
    update_request.version = int(config.get('version', 0))

    for widget in config['widgets']:
        w = update_request.widgets.add()
        ParseDict(widget, w)

    ParseDict(config['parametrization'], update_request.parametrization)

    try:
        update_response = stub.Update(request=update_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('updating dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    return update_response


def copy_dashboard_as_service_dashboard(args, creds, channel):
    dashboard = get_dashboard(args, creds, channel, args.dashboard_id, args.environment)
    if args.debug:
        print("=== SOURCE DASHBOARD DEFINITION")
        print(proto_to_text(dashboard))

    service_dashboard = get_service_dashboard(args, creds, channel, args.service_dashboard_id)
    if args.debug:
        print("=== CURRENT SERVICE DASHBOARD DEFINITION")
        print(proto_to_text(service_dashboard))

    copy_request = service_dashboard_service_pb2.SyncServiceDashboardRequest()
    copy_request.service_dashboard_id = args.service_dashboard_id
    copy_request.dashboard_id = args.dashboard_id

    stub = service_dashboard_service_pb2_grpc.ServiceDashboardServiceStub(channel)

    try:
        update_response = stub.Sync(request=copy_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('copying service dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    if args.debug:
        print("=== NEW SERVICE DASHBOARD DEFINITION")
        print(proto_to_text(update_response))

    print("Dashboard {} successfully copied to service dashboard {}".format(args.dashboard_id, args.service_dashboard_id))


def delete_service_dashboard(args, creds, channel):
    stub = service_dashboard_service_pb2_grpc.ServiceDashboardServiceStub(channel)

    delete_request = service_dashboard_service_pb2.DeleteServiceDashboardRequest()
    delete_request.service_dashboard_id = args.service_dashboard_id

    try:
        get_response = stub.Delete(request=delete_request, metadata=creds["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('deleting service dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    return get_response


def sync_service_dashboard(args, creds, channel, creds1, channel1):
    regex_query = re.compile(r"({.+})(?:,|\)|$)", re.IGNORECASE)
    regex_query_dict = re.compile(r"""(\w+)=['"]([^"']+)['"]""", re.IGNORECASE)

    service_dashboard = get_service_dashboard(args, creds, channel, args.service_dashboard_id)

    if args.debug:
        print("=== SOURCE SERVICE DASHBOARD DEFINITION")
        print(proto_to_text(service_dashboard))

    dashboard = get_dashboard(args, creds1, channel1, args.dashboard_id, args.environment1)

    if args.debug:
        print("=== TARGET DASHBOARD DEFINITION")
        print(proto_to_text(dashboard))

    if args.environment1 in ['prod', 'preprod']:
        stub = external_dashboard_service_pb2_grpc.DashboardServiceStub(channel1)
        update_request = external_dashboard_service_pb2.UpdateDashboardRequest()
    else:
        stub = internal_dashboard_service_pb2_grpc.DashboardServiceStub(channel1)
        update_request = internal_dashboard_service_pb2.UpdateDashboardRequest()

    update_request.dashboard_id = dashboard.id
    update_request.name = service_dashboard.name
    update_request.description = 'Sync from service dashboard {}'.format(service_dashboard.id)
    update_request.version = int(dashboard.version)
    for widget in service_dashboard.widgets:
        w = update_request.widgets.add()
        widget_params = MessageToDict(widget)
        if 'chart' in widget_params:
            for chart_query in widget_params['chart']['queries']['targets']:
                fixed_query = chart_query['query']
                for q in regex_query.findall(chart_query['query']):
                    _q = dict(regex_query_dict.findall(q))
                    _q['project'] = 'internal-mdb'
                    _q['service'] = 'mdb'
                    resource_type = _q.get('resource_type', None)
                    resource_id = _q.get('resource_id', None)
                    if resource_type == 'cluster':
                        _q[resource_type] = 'mdb_{}'.format(resource_id)
                    elif resource_type:
                        _q[resource_type] = resource_id
                    if 'host' not in _q.keys():
                        _q['host'] = "*"
                    if 'dc' not in _q.keys():
                        _q['dc'] = "by_host"
                    if 'node' not in _q.keys():
                        _q['node'] = "by_host"
                    for tag in ['folderId', 'resource_type', 'resource_id']:
                        if tag in _q.keys():
                            _ = _q.pop(tag)
                    fixed_query = fixed_query.replace(q, '{{{}}}'.format(', '.join(['{}="{}"'.format(k, v) for k, v
                                                                                    in _q.iteritems()])))
                chart_query['query'] = fixed_query
        ParseDict(widget_params, w)

    if args.config_path:
        config = load_config(args.config_path)
        parametrization = config.get('parametrization', {})
    else:
        parametrization = MessageToDict(service_dashboard.parametrization)
        print(parametrization)
        if args.environment1 == 'porto':
            parametrization = MessageToDict(service_dashboard.parametrization)
            new_params = []
            for p in parametrization['parameters']:
                _ = p['labelValues'].pop('folderId')
                p['labelValues']['project_id'] = "internal-mdb"
                new_params.append(p)
            parametrization['selectors'] = "{service='mdb'}"
            parametrization['parameters'] = new_params
    ParseDict(parametrization, update_request.parametrization)

    try:
        update_response = stub.Update(request=update_request, metadata=creds1["metadata"])
    except grpc.RpcError as e:
        print_grpc_error('updating target dashboard', e)
        if args.debug:
            raise e
        sys.exit(1)

    if args.debug:
        print("=== NEW TARGET DASHBOARD DEFINITION")
        print(proto_to_text(update_response))

    print("Dashboard {} from {} successfully copied to {} dashboard {} ".format(service_dashboard.id, args.environment,
                                                                                    args.environment1, dashboard.id))


def main():
    args = parse_args()

    creds = prepare_creds(args.environment)

    channel = create_channel(args.environment, creds)

    if args.command == 'get':
        if args.dashboard_id is None and args.service_dashboard_id is None:
            print("Command 'get' requires '--dashboard-id' or '--service-dashboard-id'")
            sys.exit(0)
        if args.environment == 'porto':
            validate_env_var(INTERNAL_TOKEN)
        else:
            validate_env_var(EXTERNAL_TOKEN)
        if args.dashboard_id:
            # print(proto_to_text(get_dashboard(creds, channel, args.dashboard_id, args.debug)))
            print(json.dumps(MessageToDict(get_dashboard(args, creds, channel, args.dashboard_id, args.environment)), indent=4))
        elif args.service_dashboard_id:
            print(json.dumps(MessageToDict(get_service_dashboard(args, creds, channel, args.service_dashboard_id)), indent=4))

    elif args.command == "create":
        if args.environment == 'prod':
            if not confirm():
                print("Exiting.")
                sys.exit(0)
        if args.environment == 'porto':
            validate_env_var(INTERNAL_TOKEN)
        else:
            validate_env_var(EXTERNAL_TOKEN)
        if args.config_path:
            config = load_config(args.config_path)
        else:
            config = {}

        if args.service is None:
            print("--service argument is empty,  creating regular dashboard")
            response = create_dashboard(args, creds, channel, config)
        else:
            print("--service argument is not empty, creating service dashboard")
            response = create_service_dashboard(args, creds, channel, config)
        response_dict = MessageToDict(response)
        if args.debug:
            print(proto_to_text(response))
        else:
            print('Id: {0}'.format(response_dict['id']))
        if args.config_path:
            print_config(args.config_path, response_dict)

    elif args.command == "list":
        if args.service is None:
            print("Command 'list' requires --service")
            sys.exit(0)
        if args.environment == 'porto':
            validate_env_var(INTERNAL_TOKEN)
        else:
            validate_env_var(EXTERNAL_TOKEN)
        list_service_dashboards(args, creds, channel)

    elif args.command == "copy":
        if args.dashboard_id is None or args.service_dashboard_id is None:
            print("Command 'copy' requires '--dashboard-id', '--service-dashboard-id'")
            sys.exit(0)

        if args.environment != 'preprod':
            if not confirm():
                print("Exiting.")
                sys.exit(0)
        if args.environment == 'porto':
            validate_env_var(INTERNAL_TOKEN)
        else:
            validate_env_var(EXTERNAL_TOKEN)

        copy_dashboard_as_service_dashboard(args, creds, channel)

    elif args.command == 'update':
        if args.environment != 'porto' and args.service is None:
            print("Command 'create' requires --service")
            sys.exit(0)

        if args.config_path is None:
            print("Command 'create' requires --config-path")
            sys.exit(0)

        if args.environment == 'prod':
            if not confirm():
                print("Exiting.")
                sys.exit(0)
        config = load_config(args.config_path)
        if args.environment == 'porto':
            validate_env_var(INTERNAL_TOKEN)
            response = update_dashboard(args, creds, channel, config, service_dashboard=False)
        else:
            validate_env_var(EXTERNAL_TOKEN)
            response = update_dashboard(args, creds, channel, config, service_dashboard=True)
        if args.debug:
            print(proto_to_text(response))

        print_config(args.config_path, MessageToDict(response))
    elif args.command == 'delete':
        if args.service_dashboard_id is None:
            print("Command 'delete' requires --service-dashboard-id")
            sys.exit(0)
        if args.environment != 'preprod':
            if not confirm():
                print("Exiting.")
                sys.exit(0)
        if args.environment == 'porto':
            validate_env_var(INTERNAL_TOKEN)
        else:
            validate_env_var(EXTERNAL_TOKEN)
        response = delete_service_dashboard(args, creds, channel)
        if args.debug:
            print(proto_to_text(response))
    elif args.command == "sync":
        if args.dashboard_id is None or args.service_dashboard_id is None:
            print("Command 'sync' requires '--dashboard-id' and '--service-dashboard-id'")
            sys.exit(0)
        if args.environment not in ['prod', 'preprod']:
            print("{} environment is not supported".format(args.environment))
            sys.exit(0)
        if args.environment1 not in ['porto']:
            print("{} environment is not supported".format(args.environment1))
            sys.exit(0)

        validate_env_var(INTERNAL_TOKEN)
        validate_env_var(EXTERNAL_TOKEN)

        creds1 = prepare_creds(args.environment1)
        channel1 = create_channel(args.environment1, creds1)
        sync_service_dashboard(args, creds, channel, creds1, channel1)

    else:
        raise RuntimeError("Unhandled command {}".format(args.command))


if __name__ == "__main__":
    main()
