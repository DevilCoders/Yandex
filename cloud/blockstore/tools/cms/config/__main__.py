import argparse
import json
import logging
import sys

from cloud.blockstore.tools.cms.lib.cms import Cms, CmsEngine
from cloud.blockstore.tools.cms.lib.conductor import get_dc_host, Conductor
from cloud.blockstore.tools.cms.lib.config import DC_LIST, CONFIG_NAMES, CLUSTERS
from cloud.blockstore.tools.cms.lib.proto import to_json, get_proto
from cloud.blockstore.tools.cms.lib.tools import parse_cms_configs, get_config_files, find_items, Scp


def get_node_type(args):
    if args.node_type is not None:
        return args.node_type
    return 'nbs_control' if args.control else 'nbs'


def get_dc_configs(dc, args):
    cms = Cms(dc, args.cluster, get_node_type(args), CmsEngine(), Conductor())

    dc_configs = parse_cms_configs(cms.get_tenant_config_items())

    host = get_dc_host(dc, args.cluster, args.control)

    fs_configs = get_config_files(host, Scp())

    if args.split:
        return {
            "files": to_json(fs_configs),
            "global_cms": to_json(dc_configs)
        }

    for k, v in fs_configs.items():
        dc_configs.setdefault(k, v)

    return to_json(dc_configs)


def get_host_configs(cms, host, args):
    if args.only_host:
        return to_json(parse_cms_configs(cms.get_host_config_items(host)))

    fs_configs = get_config_files(host, Scp())

    if args.split:
        host_configs = parse_cms_configs(cms.get_host_config_items(host))
        dc_configs = parse_cms_configs(cms.get_tenant_config_items())

        return {
            "files": to_json(fs_configs),
            "global_cms": to_json(dc_configs),
            "host_cms": to_json(host_configs)
        }

    host_configs = parse_cms_configs(cms.get_config_items(host))

    for k, v in fs_configs.items():
        host_configs.setdefault(k, v)

    return to_json(host_configs)


def get_dc(host, cluster):
    if cluster == 'hw-nbs-lab':
        return 'sas'
    elif cluster == 'hw-nbs-dev-lab':
        return 'myt'
    elif cluster == 'hw-nbs-stable-lab':
        return 'sas'

    p = 'nbs-control-'
    if host.startswith(p):
        host = host[len(p):]
    dc = host[:3]
    assert(dc in DC_LIST)
    return dc


def view_config(args):
    result = {}

    if args.host:
        for dc in set([get_dc(host, args.cluster) for host in args.host]):
            cms = Cms(dc, args.cluster, get_node_type(args), CmsEngine(), Conductor())
            for host in [host for host in args.host if dc == get_dc(host, args.cluster)]:
                result[host] = get_host_configs(cms, host, args)
    else:
        for dc in set(args.dc or DC_LIST):
            result[dc] = get_dc_configs(dc, args)

    json.dump(result, sys.stdout, indent=4)


def update_hosts_config(args, message):
    for dc in set([get_dc(host, args.cluster) for host in args.host]):
        cms = Cms(dc, args.cluster, get_node_type(args), CmsEngine(), Conductor())
        for host in [host for host in args.host if dc == get_dc(host, args.cluster)]:
            current = cms.get_host_config_items(host)
            items = find_items(current, set([args.config_name]))
            cms.update_host_config_items(host, [message], args.cookie)
            cms.delete_config_items(items)


def update_dc_config(dc, args, message):
    cms = Cms(dc, args.cluster, get_node_type(args), CmsEngine(), Conductor())

    current = cms.get_tenant_config_items()
    items = find_items(current, set([args.config_name]))
    cms.update_dc_config_items(message, args.cookie)
    cms.delete_config_items(items)


def update_config(args):
    message = get_proto(args.proto, args.json, args.config_name)

    if args.host:
        update_hosts_config(args, message)
        return

    for dc in set(args.dc or DC_LIST):
        update_dc_config(dc, args, message)


def remove_config(args):
    to_delete = set(args.config_name)

    if args.host:
        for dc in set([get_dc(host, args.cluster) for host in args.host]):
            cms = Cms(dc, args.cluster, get_node_type(args), CmsEngine(), Conductor())
            items = []
            for host in set([host for host in args.host if dc == get_dc(host, args.cluster)]):
                current = cms.get_host_config_items(host)
                items += find_items(current, to_delete)
            cms.delete_config_items(items)
        return

    for dc in set(args.dc or DC_LIST):
        cms = Cms(dc, args.cluster, get_node_type(args), CmsEngine(), Conductor())
        current = cms.get_tenant_config_items()
        items = find_items(current, to_delete)
        cms.delete_config_items(items)


def prepare_logging(args):
    log_level = logging.ERROR

    if args.silent:
        log_level = logging.INFO
    elif args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))

    logging.basicConfig(
        stream=sys.stderr,
        level=log_level,
        format="[%(levelname)s] [%(asctime)s] %(message)s")


def main():

    def add_common(p):
        p.add_argument('--cluster', type=str, choices=CLUSTERS, default=CLUSTERS[0])
        p.add_argument('--control', action='store_true')
        p.add_argument('--node-type', type=str, default=None)

        target = p.add_mutually_exclusive_group()
        target.add_argument('--dc', metavar="DC", nargs='*', choices=DC_LIST)
        target.add_argument("--host", metavar="HOST", nargs='*')

    parser = argparse.ArgumentParser()

    parser.add_argument("-s", '--silent', help="silent mode", default=0, action='count')
    parser.add_argument("-v", '--verbose', help="verbose mode", default=0, action='count')

    subparser = parser.add_subparsers(title='actions', dest="subparser_name", required=True)

    view = subparser.add_parser('view')
    update = subparser.add_parser('update')
    remove = subparser.add_parser('remove')

    view.add_argument('--split', action='store_true', help='split output by sources (filesystem, cms)')
    view.add_argument('--only-host', action='store_true', help='get only host-specific configs (cms only)')
    add_common(view)

    update.add_argument("--config-name", type=str, choices=CONFIG_NAMES.keys(), required=True)
    update.add_argument("--proto", type=str, required=True)
    # update.add_argument('--merge', action='store_true')
    update.add_argument('--json', action='store_true')
    update.add_argument("--cookie", type=str, default=None)
    add_common(update)

    remove.add_argument("--config-name", metavar="NAME", nargs='+', type=str, choices=CONFIG_NAMES.keys(), required=True)
    add_common(remove)

    args = parser.parse_args()

    prepare_logging(args)

    # TODO: resolve dc list from host group
    if args.cluster == 'hw-nbs-lab':
        args.dc = ['sas']
    elif args.cluster == 'hw-nbs-dev-lab':
        args.dc = ['myt']
    elif args.cluster == 'hw-nbs-stable-lab':
        args.dc = ['sas']

    if args.subparser_name == "view":
        return view_config(args)

    if args.subparser_name == "update":
        return update_config(args)

    if args.subparser_name == "remove":
        return remove_config(args)


if __name__ == '__main__':
    main()
