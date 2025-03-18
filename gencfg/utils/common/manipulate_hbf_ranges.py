#!/skynet/python/bin/python
"""
    Script to manipulate HBF ranges and HBF aliases.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import config

import re
import json
import logging
import collections

import gaux.aux_abc
import core.argparse.types as argparse_types

from core.argparse.parser import ArgumentParserExt

from core.db import CURDB
from gaux.aux_racktables import Racktables
from gaux.aux_hbf import hbf_group_macro_name


GENCFG_GROUP_MACRO_PTRN = re.compile(r'_GENCFG_(.*)_')


class Action(object):
    SHOW_GROUPS_STATS = 'show_groups_stats'
    SYNC_MACROS_INFO = 'sync_macros_info'
    SYNC_PROJECT_IDS = 'sync_project_ids'
    UPDATE_RANGES_ACL = 'update_ranges_acl'
    REPLACE_ALIASES = 'replace_aliases'
    ALL = [SHOW_GROUPS_STATS, SYNC_MACROS_INFO, SYNC_PROJECT_IDS, UPDATE_RANGES_ACL, REPLACE_ALIASES]


def is_has_unknown_owners(rt_macro_owners):
    for owner in rt_macro_owners:
        if owner.startswith('abc:') and not gaux.aux_abc.exists_service_scope_slug(owner):
            return True
    return False


def is_not_default(macro_name):
    return macro_name and macro_name != '_GENCFG_SEARCHPRODNETS_ROOT_'


def load_hbf_macros_owners(racktabes=None):
    racktabes = racktabes or Racktables(config.get_default_oauth())
    return racktabes.macros_owners()


def load_hbf_macros_project_ids(racktabes=None):
    racktabes = racktabes or Racktables(config.get_default_oauth())
    rt_macros = racktabes.list_macros()

    macros_project_ids = collections.defaultdict(set)
    project_ids_macros = collections.defaultdict(set)
    for macro_name, macro_data in rt_macros.items():
        for project_id_data in macro_data['ids']:
            macros_project_ids[macro_name].add(int(project_id_data['id'], 16))
            project_ids_macros[int(project_id_data['id'], 16)].add(macro_name)
    return macros_project_ids, project_ids_macros


def get_groups_stats(db=CURDB, racktables=None):
    _, project_ids_macros = load_hbf_macros_project_ids(racktables)

    groups_stats = collections.defaultdict(dict)

    for group in db.groups.get_groups():
        group_project_ids = [group.card.properties.hbf_project_id]
        group_project_ids.extend(group.card.properties.hbf_old_project_ids)

        for project_id in group_project_ids:
            groups_stats[group.card.name][project_id] = None
            macros_project_id = project_ids_macros.get(project_id, [])
            groups_stats[group.card.name][project_id] = list(macros_project_id)

    return groups_stats


def get_unsync_project_ids(db=CURDB, racktables=None):
    groups_stats = get_groups_stats(db, racktables)
    project_ids_create = collections.defaultdict(dict)
    project_ids_move = collections.defaultdict(dict)

    for group_name, hbf_data in groups_stats.items():
        group = db.groups.get_group(group_name)

        for project_id, macro_names in hbf_data.items():
            # Set used ALL new project_id (for control duplicates in racktables)
            if not macro_names:
                if is_not_default(group.card.properties.hbf_parent_macros):
                    project_ids_create[group_name][project_id] = group.card.properties.hbf_parent_macros
                else:
                    project_ids_create[group_name][project_id] = db.hbfranges.get_range_by_project_id(project_id).name

            # Export ALL group project_id to hbf alias (hbf_parent_macros) (for transfer between ranges)
            elif is_not_default(group.card.properties.hbf_parent_macros):
                if group.card.properties.hbf_parent_macros not in macro_names:
                    project_ids_move[group_name][project_id] = group.card.properties.hbf_parent_macros

    return project_ids_create, project_ids_move


def upload_project_ids(project_ids_create, project_ids_move, racktables=None):
    racktables = racktables or Racktables(config.get_default_oauth())

    for group_name, project_ids in project_ids_create.items():
        for project_id, macro_name in project_ids.items():
            try:
                racktables.create_network(macro_name, int(project_id))
                logging.info('Uploaded create {} {} -> {}'.format(group_name, project_id, macro_name))
            except Exception as e:
                logging.error('Skip upload create {} {} -> {} (because {}: {})'.format(
                    group_name, project_id, macro_name, type(e), e
                ))

    for group_name, project_ids in project_ids_move.items():
        for project_id, macro_name in project_ids.items():
            try:
                racktables.move_network(macro_name, int(project_id))
                logging.info('Uploaded move {} {} -> {}'.format(group_name, project_id, macro_name))
            except Exception as e:
                logging.error('Skip upload move {} {} -> {} (because {}: {})'.format(
                    group_name, project_id, macro_name, type(e), e
                ))


def sync_macros_info(db=CURDB, racktables=None):
    racktables = racktables or Racktables(config.get_default_oauth())
    rt_macros = racktables.list_macros()

    for macro_name, macro_data in rt_macros.items():
        rt_macro_parent = macro_data['parent'].encode('utf8') if macro_data['parent'] is not None else None
        rt_macro_desc = macro_data['description'].encode('utf8').replace('\r\n', '\n').strip()
        rt_macro_owners = [x.encode('utf8').replace('svc_', 'abc:') for x in macro_data['owners']]
        rt_macro_owners = [x for x in rt_macro_owners if not x.startswith('abc:') or db.abcgroups.has_service_slug(x)]

        # Create new macro from RT in GenCfg DB
        if not db.hbfmacroses.has_hbf_macros(macro_name):
            new_macro = db.hbfmacroses.add_hbf_macros(
                macro_name,
                rt_macro_desc,
                rt_macro_owners,
                rt_macro_parent,
                -1  # Disable generate project_id for macro
            )
            new_macro.skip_owners_check = True
            new_macro.group_macro = True if GENCFG_GROUP_MACRO_PTRN.match(macro_name) else False
            new_macro.external = not new_macro.group_macro
            db.hbfmacroses.mark_as_modified()
            logging.info('Added new macro form RT {}({}, {})'.format(
                macro_name, rt_macro_parent, ','.join(rt_macro_owners)
            ))

        # Update macro properties in GenCfg DB
        else:
            gencfg_macro = db.hbfmacroses.get_hbf_macros(macro_name)
            gencfg_macro_parent = gencfg_macro.parent_macros
            gencfg_macro_desc = gencfg_macro.description.encode('utf8').replace('\r\n', '\n').strip()
            gencfg_macro_owners = [x.encode('utf8') for x in gencfg_macro.owners]

            if rt_macro_parent != gencfg_macro_parent:
                gencfg_macro.parent_macros = rt_macro_parent
                db.hbfmacroses.mark_as_modified()
                logging.info(u'Update macro parent from RT {}({} -> {})'.format(
                    macro_name, gencfg_macro_parent, rt_macro_parent
                ))

            if set(rt_macro_owners) != set(gencfg_macro_owners):
                if not is_has_unknown_owners(rt_macro_owners):
                    gencfg_macro.owners = rt_macro_owners
                    db.hbfmacroses.mark_as_modified()
                    logging.info(u'Update macro owners from RT {}({} -> {})'.format(
                        macro_name, ','.join(gencfg_macro_owners), ','.join(rt_macro_owners)
                    ))
                else:
                    logging.error(u'In RT data for macro {} has unknown owners {}'.format(
                        macro_name, ','.join(rt_macro_owners)
                    ))

            if rt_macro_desc != gencfg_macro_desc:
                gencfg_macro.description = rt_macro_desc
                db.hbfmacroses.mark_as_modified()
                logging.info(u'Update macro description from RT {}'.format(macro_name))

    #for gencfg_macro in db.hbfmacroses.get_hbf_macroses():
    #    # Remove removed from RT macro from GenCfg DB
    #    if gencfg_macro.name not in rt_macros:
    #        db.hbfmacroses.remove_hbf_macros(gencfg_macro.name)
    #        logging.info(u'Remove macro {}'.format(gencfg_macro.name))


def sync_project_ids(db=CURDB, racktables=None, upload=False, upload_limit=20):
    racktables = racktables or Racktables(config.get_default_oauth())
    project_ids_create, project_ids_move = get_unsync_project_ids(db, racktables)

    if upload:
        if upload_limit > 0 and len(project_ids_move) > upload_limit:
            raise RuntimeError('More than {} move changes for upload to RT.'.format(upload_limit))
        elif project_ids_create or project_ids_move:
            logging.info('Upload...')
            upload_project_ids(project_ids_create, project_ids_move)
            logging.info('Upload done.')
        else:
            logging.info('Nothing to upload.')
    else:
        print('CREATE PROJECT_ID\n{}'.format(json.dumps(project_ids_create, indent=4)))
        print('MOVE PROJECT_ID\n{}'.format(json.dumps(project_ids_move, indent=4)))


def update_ranges_acl(db=CURDB, racktables=None):
    macros_owners = load_hbf_macros_owners(racktables)

    for hbf_range in db.hbfranges.get_ranges():
        new_acl = sorted(list({x['name'] for x in macros_owners.get(hbf_range.name, {}).get('owners', [])}))
        if new_acl != hbf_range.acl:
            logging.info('{} changed acl {} -> {}'.format(
                hbf_range.name, hbf_range.acl, new_acl
            ))
            hbf_range.acl = new_acl
            db.hbfranges.mark_as_modified()


def replace_aliases(db=CURDB, replace_aliases_filepath=None, prefix='SAS_', perc_limit=0.1):
    prefix = prefix if prefix != '*' else ''

    replace_aliases = set()
    with open(replace_aliases_filepath) as rfile:
        replace_aliases = json.load(rfile)

    replace_mapping = {}
    for replace_item in replace_aliases:
        replace_mapping[replace_item['removed_macro']] = {
            'project_ids': [int(x[2:], 16) for x in replace_item['pid_list']],
            'target_alias': str(replace_item['moved_to']) if replace_item['moved_to'] != '_GENCFG_SEARCHPRODNETS_ROOT_' else None,
        }

    assert len(replace_aliases) == len(replace_mapping)

    all_group = db.groups.get_groups()
    limit = int(len(all_group) * perc_limit)

    logging.info('Changes limit set on {} groups'.format(limit))
    logging.info('Only groups with prefix `{}` will be handled'.format(prefix))

    for group in all_group:
        if limit <= 0:
            logging.info('Limit exceded')
            break

        # Separate changes by DC
        if not group.card.name.startswith(prefix):
            continue

        macro = group.card.properties.hbf_parent_macros
        if macro not in replace_mapping or \
                group.card.properties.hbf_project_id not in replace_mapping[macro]['project_ids']:
            continue

        group.card.properties.hbf_parent_macros = replace_mapping[macro]['target_alias']
        group.mark_as_modified()
        limit -= 1

        logging.info('Change {}: {} -> {}'.format(
            group.card.name, macro, group.card.properties.hbf_parent_macros
        ))


def get_parser():
    parser = ArgumentParserExt(description='Tool for sync and manage hbf data.')
    parser.add_argument('-a', '--action', type=str, choices=Action.ALL,
                        help='Action to execute.')
    parser.add_argument('--db', type=argparse_types.gencfg_db, default=CURDB,
                        help='Optional. Path to db')
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Optional. Increase output verbosity (maximum 1)')
    parser.add_argument('-y', '--apply', action='count', default=0,
                        help='Optional. Write changes on disk.')
    parser.add_argument('--upload', action='count', default=0,
                        help='Optional. Upload changes to RackTables. (include --apply)')
    parser.add_argument('--upload-limit', type=int, default=20,
                        help='Optional. Limit for upload changes to RackTable. (-1 for unlimited)')
    parser.add_argument('--prefix', type=str, default='',
                        help='Prefix group or macro name for selective execution.')
    parser.add_argument('--replace-aliases-filepath', type=str, default=None,
                        help='File with replacing aliases.')
    parser.add_argument('--replace-aliases-prefix', type=str, default='SAS_',
                        help='Prefix for grpup name ("SAS_","MAN_","VLA_","*").')

    return parser


def normalize(option):
    if options.action == 'replace_aliases' and options.replace_aliases_filepath is None:
        raise ValueError('For action `replace_aliases` need specify --replace-aliases-filepath')


def main(options):
    if options.verbose:
        logging.basicConfig(level=logging.INFO)
    else:
        logging.basicConfig(level=logging.ERROR)

    if options.action == Action.SHOW_GROUPS_STATS:
        group_stats = get_groups_stats(options.db)
        print(json.dumps(group_stats, indent=4))
    elif options.action == Action.SYNC_MACROS_INFO:
        sync_macros_info(options.db)
    elif options.action == Action.SYNC_PROJECT_IDS:
        sync_project_ids(options.db, upload=options.upload, upload_limit=options.upload_limit)
    elif options.action == Action.UPDATE_RANGES_ACL:
        update_ranges_acl(options.db)
    elif options.action == Action.REPLACE_ALIASES:
        replace_aliases(options.db, options.replace_aliases_filepath, options.replace_aliases_prefix)

    if options.apply or options.upload:
        options.db.hbfmacroses.update(smart=True)
        options.db.hbfranges.update(smart=True)
        options.db.groups.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    main(options)

