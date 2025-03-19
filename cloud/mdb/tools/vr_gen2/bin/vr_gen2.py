import argparse
import logging
import os

import sys
import yaml
from cloud.mdb.tools.vr_gen2.internal.data_sources import FlavorSource, GeoIdSource, DiskTypeIdSource
from cloud.mdb.tools.vr_gen2.internal.models.resource import ResourceDumper
from cloud.mdb.tools.vr_gen2.internal.product import generate_resources
from cloud.mdb.tools.vr_gen2.internal.schema import load_resource_definitions

from .logs import conf_logging


def produce(base_dir: str, output: str, res: str, disk: str, flavor: str, geo: str):
    logger = logging.getLogger(__name__)
    with open(os.path.join(base_dir, res), 'r') as fp:
        defs = load_resource_definitions(fp.read())

    with open(os.path.join(base_dir, geo), 'r') as fp:
        geo_def = fp.read()
    geo = GeoIdSource(geo_def)

    with open(os.path.join(base_dir, flavor), 'r') as fp:
        flavor_def = fp.read()
    flavor = FlavorSource(flavor_def)

    with open(os.path.join(base_dir, disk), 'r') as fp:
        disk_def = fp.read()
    disk = DiskTypeIdSource(disk_def)

    result = generate_resources(resource_definition=defs, geo_source=geo, flavor_source=flavor, disk_source=disk)
    logger.info('Generated %d entries', len(result))

    with open(output, 'w') as destfp:
        destfp.write(yaml.dump(data=result, Dumper=ResourceDumper))


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('action', help='action', type=str, choices=['produce'])
    parser.add_argument('-d', '--base_dir', help='base dir', type=str, default='/etc/yandex/metadb/resources/')
    parser.add_argument(
        '-o',
        '--output',
        type=str,
        help='path to output file',
        default='/etc/yandex/metadb/resources/valid_resources.yaml',
    )
    parser.add_argument('--res', help='resource definitions (former vr_gen.conf)', type=str, default='vr_gen.yaml')
    parser.add_argument('--disk', help='disk definitions', type=str, default='disk_type.yaml')
    parser.add_argument('--flavor', help='flavor definitions', type=str, default='flavor.yaml')
    parser.add_argument('--geo', help='geo definitions', type=str, default='geo.yaml')

    args = parser.parse_args()
    conf_logging()
    if args.action == 'produce':
        produce(args.base_dir, args.output, args.res, args.disk, args.flavor, args.geo)
        sys.exit(0)
    sys.exit(1)
