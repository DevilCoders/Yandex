import os
import re
import socket
import argparse

from antiadblock.libs.deploy_unit_resolver.lib import get_fqdns, get_ips

HOSTNAME_TEMPL = r'(.*?\.)?(?P<fqdn>(?P<pod_id>\w+)\.(?P<dc>\w+)\.yp-c\.yandex\.net)'
# example: sas3-7104-1.fgk3vj2vpld5xd6y.sas.yp-c.yandex.net
HOSTNAME_RE = re.compile(HOSTNAME_TEMPL)
CLUSTERS = ['sas', 'vla', 'man', 'iva', 'myt']

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--deploy_stage', type=str)
    parser.add_argument('--deploy_units', type=str)
    parser.add_argument('--clusters', type=str, help="for example sas,vla")

    parser.add_argument('--mkfile', action='store_true')
    parser.add_argument('--filename', type=str, help="path to the file for the fqdns")

    parser.add_argument('--print_fqdns', action='store_true')
    parser.add_argument('--add_port', type=str)

    parser.add_argument('--print_hostname', action='store_true')
    parser.add_argument('--print_cluster', action='store_true')

    args = parser.parse_args()

    m = HOSTNAME_RE.match(socket.gethostname())

    clusters = args.clusters.split(',') if args.clusters else [m.group('dc')]
    for cluster in clusters:
        assert cluster in CLUSTERS
    deploy_stage = args.deploy_stage or os.getenv('DEPLOY_STAGE_ID')
    assert deploy_stage is not None
    deploy_units = args.deploy_units.split(',') if args.deploy_units else [os.getenv('DEPLOY_UNIT_ID')]
    port = args.add_port

    endpoint_fqdns = get_fqdns(deploy_stage, deploy_units, clusters)
    fqdns = map(lambda f: f"{f}{(':' + port) if port else ''}", endpoint_fqdns)

    if args.mkfile:
        filename = args.filename if args.filename else os.path.join(os.getenv('ES_PATH_CONF', '/usr/share/elasticsearch/config'), "unicast_hosts.txt")
        with open(filename, "w") as f:  # Use file to refer to the file object
            f.write('\n'.join(fqdns))

    if args.print_fqdns:
        print(",".join(fqdns))

    if args.print_hostname:
        print(m.group('fqdn'))

    if args.print_cluster:
        print(cluster)
