"""
Ensure properties of transfer manager entities correspond to the target state defined in yaml conf file.

Use following algorithm:
 - Load desired state configuration for transfers
 - Get current status for all present transfers
 - For each transfer, retrieve relevant pieces of information:
   - consumer
   - field data
   - format
   - topic
 - using members of TransferSpecifier (a tuple of topic, database and cluster) as keys,
   compare the difference in TM and desired states.
 - output a diff. If dry run was requested, terminate the program.
 - For every transfer where a change has been detected:
    - create a pair of src, dst endpoints;
    - drop the old one.
    - merge them in a new transfer
    Note that old endpoints remain in place.
"""
from collections import namedtuple
from functools import partial
from copy import deepcopy
import difflib
import logging
import string
from typing import List

import yaml

from .api import TM, getter

LOG = logging.getLogger('tm-util')

SPEC_TEMPLATE = {
    'src': {
        'connection': {
            'cluster': 'logbroker',
        },
        'consumer': None,
        'fields': [],
        'format': 'tskv',
        'topic': None,
    },
    'dst': {
        'database': None,
        'password': {
            'secret': {},
        },
        'user': None,
        'alt_names': {
            'from_name': None,
            'to_name': None,
        },
        'prefer_json': True,
    },
    'folder_id': None,
}

LB_SOURCE_TEMPLATE = {
    'logbroker_source': {
        'add_system_cols': False,
        'consumer': None,
        'fields': None,
        'format': 'tskv',
        'topic': None,
        'allow_ttl_rewind': True,
    },
}

# This tuple uniquely identifies a connector both in the config and in list handle.
TransferSpecifier = namedtuple('TransferSpecifier', ('topic', 'mdb_cluster_id', 'database'))

ALREADY_USED_NAMES = set()

class TransferView:
    def __init__(self, transfer):
        self.id = transfer['id']
        lb_source = transfer['source'].get('logbroker_v2_source', transfer['source'].get('logbroker_source'))
        self.topic = lb_source.get('topic', 'NO_TOPIC')
        self.status = transfer['status']
        self.dst_id = transfer['target']['id']
        self.src_id = transfer['source']['id']

    def __repr__(self):
        return ' '.join((
            self.id,
            self.topic,
            self.status))

    def __hash__(self):
        return hash(self.id)


class EndpointView:
    def __init__(self, endpoint):
        self.id = endpoint['id']
        self.name = endpoint['name']
        self.type = endpoint['type']

    def __repr__(self):
        return ' '.join((
            self.id,
            self.name,
            self.type))


    def __hash__(self):
        return hash(self.id)

def get_transfers_specs(config: dict) -> list:
    try:
        return deepcopy(config['transfers'])
    except KeyError as key:
        raise RuntimeError(f'missing required config field: {key}')


def get_transfers_tm(api: TM) -> list:
    return getter(api.list_transfers, 'transfers')


def get_endpoints_tm(api: TM) -> list:
    return getter(api.list_endpoints, 'endpoints')


def get_logs_tm(api: TM, entity: str) -> list:
    return getter(api.list_logs, 'log_events', entity_id=entity)


def gen_alt_names(service: str, topic: str) -> List[dict]:
    """
    Generate renaming policy for CH tables
    Uses topic names by default. We want to use service name
    """
    def _to_table_name(name: str, tm_879_bug=False) -> str:
        if tm_879_bug:
            return name.replace('/', '_')
        return name.replace('/', '_').replace('-', '_')

    alt_names = []
    transformers = [
        _to_table_name,
        partial(_to_table_name, tm_879_bug=True),
    ]
    patterns = ['{service}', '{service}_unparsed']
    for pattern in patterns:
        for xform in transformers:
            alt_names.append(
                {
                    'from_name': pattern.format(service=xform(topic)),
                    'to_name': pattern.format(service=service),
                },
            )
    return alt_names


def response_to_spec(resp: dict) -> dict:
    """
    Receive a raw response, return a (would-be) specification.
    Must correspond to fields in SPEC_TEMPLATE
    """
    return {
        'folder_id': resp['folder_id'],
        'src': {
            'connection': get_logbroker_options(resp)['connection'],
            'consumer': get_logbroker_options(resp)['consumer'],
            'topic': get_topic_from_resp(resp),
            'format': get_logbroker_format(resp),
            'fields': get_logbroker_fields(resp),
        },
        'dst': {
            'database': resp['target']['clickhouse_target']['connection']['connection_options']['database'],
            'mdb_cluster_id': resp['target']['clickhouse_target']['connection']['connection_options']['mdb_cluster_id'],
            'user': resp['target']['clickhouse_target']['connection']['connection_options']['user'],
        },
        # 'folder_id': resp['target']['folder_id'],
    }


def compare_transfer_specs(current, requested) -> str:
    """
    Produces a textual diff of two structures.
    """
    # Listing response does not return 1:1 presentation os specification.
    spec_no_passwd = redact_spec(requested)
    one_yaml = yaml.safe_dump(current, default_flow_style=False)
    another_yaml = yaml.safe_dump(spec_no_passwd, default_flow_style=False)
    return ''.join(difflib.unified_diff(
        one_yaml.splitlines(keepends=True),
        another_yaml.splitlines(keepends=True),
        'Current TM configuration',
        'Requested additions'))


def get_transfer_by_props(transfer_list: list, xfer_id: TransferSpecifier) -> dict:
    for xfer in transfer_list:
        try:
            ch_options = xfer['target']['clickhouse_target']['connection']['connection_options']
            criteria = [
                get_topic_from_resp(xfer) == xfer_id.topic,
                ch_options['mdb_cluster_id'] == xfer_id.mdb_cluster_id,
                ch_options['database'] == xfer_id.database,
            ]
            if all(criteria):
                return xfer
        except KeyError:
            continue


def get_logbroker_options(response):
    if 'logbroker_v2_source' in response['source']:
        return response['source']['logbroker_v2_source']
    return response['source']['logbroker_source']


def get_topic_from_resp(resp):
    if 'logbroker_v2_source' in resp['source']:
        assert len(resp['source']['logbroker_v2_source']['topics']) == 1
        return resp['source']['logbroker_v2_source']['topics'][0]
    return resp['source']['logbroker_source']['topic']


def get_logbroker_format(response):
    if 'logbroker_v2_source' in response['source']:
        return response['source']['logbroker_v2_source']['parser']['generic_parser']['format'].lower()
    return response['source']['logbroker_source']['format']


def get_logbroker_fields(response):
    if 'logbroker_v2_source' in response['source']:
        return response['source']['logbroker_v2_source']['parser']['generic_parser']['data_schema']['fields']['fields']
    return response['source']['logbroker_source']['fields']


def merge(src, dst):
    for key, value in src.items():
        if isinstance(value, dict):
            node = dst.setdefault(key, {})
            merge(value, node)
        else:
            dst[key] = value
    return dst


def form_create_spec(transfer_conf, config) -> dict:

    req = deepcopy(SPEC_TEMPLATE)
    merge(transfer_conf['src'], req['src'])
    merge(transfer_conf['dst'], req['dst'])
    req.update({
        'folder_id': config.get('folder_id'),
        'src': {
            **req.get('src', {}),
            'connection': {
                'cluster': config.get('logbroker', 'logbroker'),
            },
        },
        'dst': {
            **req.get('dst', {}),
            'alt_names': gen_alt_names(
                transfer_conf['service'], transfer_conf['src']['topic']),
        }
    })
    LOG.debug(req)
    return req


def normalize_names(name):
    if len(name) > 63:
        LOG.warning('Name should be in range [3.. 63], chomp long name: {} -> {}'.format(name, name[:63]))
        name = name[:63]

    if name[-1] not in string.ascii_lowercase:
        chomped_name = name.rstrip(name[-1])
        LOG.warning('Name should end with alphabet character, chomp name: {} -> {}'.format(name, chomped_name))
        name = chomped_name

    repeat_prefix = ord('a')
    while name in ALREADY_USED_NAMES:
        prefixed_name = name[:-1] + chr(repeat_prefix)
        repeat_prefix += 1
        LOG.warning('Name {} already used, regenerate name to {}'.format(name, prefixed_name))
        name = prefixed_name

    ALREADY_USED_NAMES.add(name)
    return name


def form_create_spec_src_logbroker(transfer_conf, config) -> dict:
    req = deepcopy(LB_SOURCE_TEMPLATE)
    merge(transfer_conf['src'], req['logbroker_source'])
    name = 'src-ep-lb-{lb}--to--ch-{ch}'.format(
            lb=transfer_conf['src']['topic'].replace('/', '-').replace('_', '-'),
            ch=transfer_conf['dst']['mdb_cluster_id'],
        )
    name = normalize_names(name)
    req.update({
        'folder_id': config.get('folder_id'),
        'name': name,
        'logbroker_source': {
            **req['logbroker_source'],
            'connection': {
                'cluster': config.get('logbroker', 'logbroker'),
            },
            'format': transfer_conf['src'].get('format', 'tskv')
        },
    })
    LOG.debug('logbroker source request: %s', req)
    return req


def form_create_spec_dst_clickhouse(transfer_conf, config) -> dict:
    name = 'dst-ep-lb-{lb}--to--ch-{ch}'.format(
            lb=transfer_conf['src']['topic'].replace('/','-').replace('_', '-'),
            ch=transfer_conf['dst']['mdb_cluster_id'],
        )
    name = normalize_names(name)
    req = {
        'name': name,
        'folder_id': config.get('folder_id'),
        'clickhouse_target': {
            'alt_names': gen_alt_names(
                transfer_conf['service'], transfer_conf['src']['topic']),
            'uploadAsJson': True,
            'flush_interval': '1s',
            'connection': {
                'connection_options': {
                    **transfer_conf['dst']
                }
            },
        }
    }
    LOG.debug('clickhouse target request: %s', req)
    return req


def form_create_spec_transfer(transfer_conf, config, source_id, target_id) -> dict:
    name = 'xfer-lb-{lb}--to--ch-{ch}'.format(
            lb=transfer_conf['src']['topic'].replace('/', '-').replace('_', '-'),
            ch=transfer_conf['dst']['mdb_cluster_id'],
        )
    req = {
        'name': normalize_names(name),
        'folder_id': config.get('folder_id'),
        'source_id': source_id,
        'target_id': target_id,
        'type': 'INCREMENT_ONLY',
    }
    LOG.debug('clickhouse target request: %s', req)
    return req


def print_diff(diffs, printer=print):
    output = []
    for xfer_id, diff, cur in diffs:
        output.append(f'# Changes for {xfer_id} (TM id: {cur.get("id")}):\n{diff}\n')
    return printer(''.join(output))


def redact_spec(spec) -> dict:
    """ Clip sensitive or data missing from spec so that it can be compared with listing response """
    data = deepcopy(spec)
    for key in ['password', 'alt_names', 'prefer_json', 'raw_password']:
        try:
            del data['dst'][key]
        except KeyError:
            pass
    return data


def get_transfer_specifier(spec):
    try:
        return TransferSpecifier(
            topic=spec['src']['topic'],
            database=spec['dst']['database'],
            mdb_cluster_id=spec['dst']['mdb_cluster_id']
        )
    except KeyError as key:
        LOG.error(f'Connector is missing field {key}. Parsed config: {spec}')
        raise RuntimeError(f'missing required field in transfer spec: {key}')


def create_transfers(transfers, config, api):
    for transfer in transfers:
        xfer_id = get_transfer_specifier(transfer)
        LOG.info(f'creating transfer for {xfer_id}')
        source_spec = form_create_spec_src_logbroker(transfer, config)
        target_spec = form_create_spec_dst_clickhouse(transfer, config)
        old_xfer_id = transfer.get('old_id')
        if old_xfer_id is not None:
            api.deactivate(old_xfer_id)
            api.transfer_wait_state(old_xfer_id, 'STOPPED', 120)
            api.delete(old_xfer_id)
            for endpoint in transfer['old_endpoints']:
                api.delete_endpoint(endpoint)
        source = api.create_endpoint(source_spec)
        target = api.create_endpoint(target_spec)
        xfer_spec = form_create_spec_transfer(transfer, config, source['id'], target['id'])
        api.create_transfer_by_id(xfer_spec)


def create(config, api: TM, dry_run=True):
    """
    Create transfer
    """
    desired = get_transfers_specs(config)
    actual = get_transfers_tm(api)
    recreate_transfers = []
    diffs = []
    unique_check_set = set()
    # Review the current state and form task list to (re)create
    for transfer in desired:
        xfer_specifier = get_transfer_specifier(transfer)
        current_transfer = get_transfer_by_props(actual, xfer_specifier) or {}
        if xfer_specifier in unique_check_set:
            raise RuntimeError(f'Connector with specifier {xfer_specifier} is defined multiple times')
        unique_check_set.add(xfer_specifier)
        # get a textual diff
        diff = compare_transfer_specs(
            response_to_spec(current_transfer) if current_transfer else {},
            form_create_spec(transfer, config))
        tm_id = current_transfer.get('id')
        if not diff:
            LOG.debug(f'No changes detected for {xfer_specifier} (TM id: {tm_id})')
            continue
        # Set id of the old connector; will replace current later
        transfer['old_id'] = tm_id
        if current_transfer:
            transfer['old_endpoints'] = [current_transfer['source']['id'], current_transfer['target']['id']]
        recreate_transfers.append(transfer)
        diffs.append((xfer_specifier, diff, current_transfer))
    print_diff(diffs)
    if dry_run:
        return
    # Recreate
    create_transfers(recreate_transfers, config, api)


def start(api: TM, xfer_id=None):
    """ Start all found transfers """
    activate_list = []
    for transfer in get_transfers_tm(api):
        if transfer['status'] not in ['RUNNING']:
            if xfer_id is None or transfer['id'] == xfer_id:
                activate_list.append(transfer['id'])

    for transfer_id in activate_list:
        LOG.info(f'starting transfer {transfer_id}')
        api.start(transfer_id)


def status(api: TM, idonly=False, xfer_id=None):
    xfer_list = []
    for transfer in get_transfers_tm(api):
        xfer = TransferView(transfer)
        if xfer_id is None or xfer_id == xfer.id:
            xfer_list.append(xfer)
    if idonly:
        print('\n'.join(x.id for x in xfer_list))
    else:
        print('\n'.join(repr(x) for x in xfer_list))


def list_endpoints(api: TM, idonly=False):
    endpoints = []
    for endpoint in get_endpoints_tm(api):
        endpoints.append(EndpointView(endpoint))
    if idonly:
        print('\n'.join(x.id for x in endpoints))
    else:
        print('\n'.join(repr(x) for x in endpoints))


def logs(api: TM, entity: str):
    for rec in reversed(get_logs_tm(api, entity=entity)):
        print(rec['created_at'], rec['message'])


def deactivate(api: TM, entity: str):
    return api.deactivate(entity)


def drop_orphans(api: TM, dry_run=False):
    used_ids = set()
    orphaned = set()
    for transfer in get_transfers_tm(api):
        xfer = TransferView(transfer)
        used_ids.add(xfer.dst_id)
        used_ids.add(xfer.src_id)
    for endpoint in get_endpoints_tm(api):
        endpoint_obj = EndpointView(endpoint)
        if endpoint_obj.id not in used_ids:
            orphaned.add(endpoint_obj)
    if dry_run:
        print('\n'.join((str(x) for x in orphaned)))
        return

    for endpoint in orphaned:
        LOG.info(f'dropping endpoint {str(endpoint)}')
        api.delete_endpoint(endpoint.id)
