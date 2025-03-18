#!venv/venv/bin/python
"""
    Script to manipulate Dispenser quoats.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import config

import json
import logging
import collections

import core.argparse.types as argparse_types

from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from gaux.aux_dispenser import Dispenser
from gaux.aux_dispenser import load_dispenser_data, calc_projects_allocated
from gaux.aux_dispenser import Mapping


class Action(object):
    SHOW_DISPENSER_QUOTAS = 'show_dispenser_quotas'
    SHOW_GENCFG_DUMP_QUOTAS = 'show_gencfg_dump_quotas'
    SHOW_ALLOCATE = 'show_allocated'
    SYNC_FROM_DISPENSER = 'sync_from_dispenser'
    SYNC_ACTUAL_QUOTAS = 'sync_actual_quotas'
    EXPORT_MAX_QUOTA = 'export_max_quota'
    APPLY_MAPPING = 'apply_mapping'
    ALL = [SHOW_DISPENSER_QUOTAS, SHOW_GENCFG_DUMP_QUOTAS, SHOW_ALLOCATE,
           SYNC_FROM_DISPENSER, SYNC_ACTUAL_QUOTAS, EXPORT_MAX_QUOTA, APPLY_MAPPING]


def show_dispenser_quotas(db=CURDB, dispenser=None):
    dispenser = dispenser or Dispenser(config.get_default_oauth())
    dispenser_data = load_dispenser_data(dispenser)

    print(json.dumps(dispenser_data['quotas'], indent=4))


def show_gencfg_dump_quotas(db=CURDB):
    print(json.dumps(db.dispenser.get_quotas(), indent=4))


def show_allocated(db=CURDB, project_key=None):
    projects_allocated = calc_projects_allocated(db, project_key)
    decoded = {}

    for project_key in projects_allocated:
        decoded[project_key] = {}
        for path_keys, value in projects_allocated[project_key].items():
            loc, seg, _, res = path_keys
            if loc not in decoded[project_key]:
                decoded[project_key][loc] = {}
            if seg not in decoded[project_key][loc]:
                decoded[project_key][loc][seg] = {}
            decoded[project_key][loc][seg][res] = value

    print(json.dumps(decoded, indent=4))


def sync_from_dispenser(db=CURDB, dispenser=None):
    dispenser = dispenser or Dispenser(config.get_default_oauth())
    dispenser_data = load_dispenser_data(dispenser)

    db.dispenser.from_json(dispenser_data)
    db.dispenser.mark_as_modified()


def sync_actual_quotas(db=CURDB, upload=False, dispenser=None):
    dispenser = dispenser or Dispenser(config.get_default_oauth())
    projects_allocated = calc_projects_allocated(db)
    dispenser_quotas = db.dispenser.get_keyed_quotas()
    leaf_project = db.dispenser.get_leaf_projects().keys()

    list_sync_state_quotas_objs = []

    # Full projects
    for project_key in projects_allocated:
        for path_keys, value in projects_allocated[project_key].items():
            loc_key, seg_key, spec_key, res_key = path_keys
            res_unit = Mapping.d_unit(res_key)
            segments = [loc_key, seg_key]

            res_actual = db.dispenser.get_sum_project_quota(
                project_key, loc_key, seg_key, spec_key, res_key
            )[res_key]['actual']
            res_max = db.dispenser.get_max_project_quota(
                project_key, loc_key, seg_key, spec_key, res_key
            )[res_key]['max']

            if res_actual == value:
                logging.info('SKIP {}:{}:{}:{} {} == {}'.format(
                    project_key, loc_key, seg_key, res_key, res_actual, value
                ))
                continue

            if res_max < value:
                logging.error('BAD {}:{}:{}:{} {} < {}'.format(
                    project_key, loc_key, seg_key, res_key, res_max, value
                ))

            if res_max > value and res_actual != value:
                logging.info('GOOD {}:{}:{}:{} {} != {}'.format(
                    project_key, loc_key, seg_key, res_key, res_actual, value
                ))

            sync_state_quotas_obj = dispenser.make_sync_state_quotas_obj(
                project_key, res_key, spec_key, segments,
                actual_value=value, actual_unit=res_unit
            )
            list_sync_state_quotas_objs.append(sync_state_quotas_obj)

    # Empty projects
    for project_key in dispenser_quotas:
        if project_key in projects_allocated or project_key not in leaf_project:
            continue

        for path_keys, quotas in dispenser_quotas[project_key].items():
            loc_key, seg_key, spec_key, res_key, _ = path_keys
            res_unit = Mapping.d_unit(res_key)
            segments = [loc_key, seg_key]

            if quotas['actual'] == 0:
                continue

            sync_state_quotas_obj = dispenser.make_sync_state_quotas_obj(
                project_key, res_key, spec_key, segments,
                actual_value=0, actual_unit=res_unit
            )
            list_sync_state_quotas_objs.append(sync_state_quotas_obj)

    # Upload
    if upload:
        if list_sync_state_quotas_objs:
            logging.info('Uploading...')
            dispenser.sync_state_quotas('gencfg', list_sync_state_quotas_objs)
            logging.info('Uploaded.')
        else:
            logging.info('Nothing to upload.')
    else:
        print(json.dumps(list_sync_state_quotas_objs, indent=4))


def export_max_quota(db=CURDB, max_quota_file=None, upload=True):
    if max_quota_file is None:
        raise ValueError('For action export_max_quota need set --max_quota_file.')

    with open(os.path.join(os.path.expanduser('~'), '.dispenser-token')) as rfile:
        token = rfile.read().strip()

    with open(max_quota_file) as rfile:
        data = json.load(rfile)

    d = Dispenser(token)

    new_max = []
    for project_keys in data:
        for project_key in project_keys.split(','):
            for loc_keys in data[project_keys]:
                for loc_key in loc_keys.split(','):
                    for seg_keys in data[project_keys][loc_keys]:
                        for seg_key in seg_keys.split(','):
                            for res_keys, value in data[project_keys][loc_keys][seg_keys].items():
                                for res_key in res_keys.split(','):
                                    spec_key = '{}-quota'.format(res_key)
                                    new_max.append(d.make_sync_state_quotas_obj(
                                        project_key, res_key, spec_key, [loc_key, seg_key],
                                        max_value=value, max_unit=Mapping.d_unit(res_key)
                                    ))

    if upload:
        data = d.sync_state_quotas('gencfg', new_max)
        print(json.dumps(data, indent=4))
    else:
        print(json.dumps(new_max, indent=4))


def apply_mapping(db=CURDB, mapping_filepath=None):
    if mapping_filepath is None:
        raise ValueError('For action `apply_mapping` key --mapping_file requared.')

    with open(mapping_filepath) as rfile:
        mapping = json.load(rfile)

    for project_key, groups in mapping.items():
        for group_name in groups:
            group = db.groups.get_group(group_name)
            group.card.dispenser.project_key = project_key
            group.mark_as_modified()


def get_parser():
    parser = ArgumentParserExt(description='Tool for sync and manage dispenser data.')
    parser.add_argument('-a', '--action', type=str, choices=Action.ALL,
                        help='Action to execute.')
    parser.add_argument('--max_quota_file', type=str, default=None,
                        help='Export max quota.')
    parser.add_argument('--mapping_file', type=str, default=None,
                        help='Mapping file.')
    parser.add_argument('--project_key', type=str, default=None,
                        help='Optional. Project key for show_allocated action.')
    parser.add_argument('--db', type=argparse_types.gencfg_db, default=CURDB,
                        help='Optional. Path to db')
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Optional. Increase output verbosity (maximum 1)')
    parser.add_argument('-y', '--apply', action='count', default=0,
                        help='Optional. Write changes on disk.')
    parser.add_argument('--upload', action='count', default=0,
                        help='Optional. Upload changes to Dispenser.')
    return parser


def main(options):
    if options.verbose == 1:
        logging.basicConfig(level=logging.INFO)
    elif options.verbose > 1:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.ERROR)

    if options.action == Action.SHOW_DISPENSER_QUOTAS:
        show_dispenser_quotas(options.db)
    elif options.action == Action.SHOW_GENCFG_DUMP_QUOTAS:
        show_gencfg_dump_quotas(options.db)
    elif options.action == Action.SHOW_ALLOCATE:
        show_allocated(options.db, options.project_key)
    elif options.action == Action.SYNC_FROM_DISPENSER:
        sync_from_dispenser(options.db)
    elif options.action == Action.SYNC_ACTUAL_QUOTAS:
        sync_actual_quotas(options.db, options.upload)
    elif options.action == Action.EXPORT_MAX_QUOTA:
        export_max_quota(options.db, options.max_quota_file, options.upload)
    elif options.action == Action.APPLY_MAPPING:
        apply_mapping(options.db, options.mapping_file)

    if options.apply:
        options.db.dispenser.update(smart=True)
        options.db.groups.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)

