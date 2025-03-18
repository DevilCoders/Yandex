#!/skynet/python/bin/python
"""Checks on hbfmacroses consistency (no cycles in tree, no removed macroses with not removed children)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re

import gencfg
from core.db import CURDB

from gaux.aux_colortext import red_text
from gaux.aux_utils import indent
from gaux.aux_hbf import hbf_group_macro_name
from gaux.aux_staff import unwrap_dpts


PATTERN = "^_GENCFG_([A-Z0-9_]+)_$"


def check_group_unknown_parent_macros():
    """Check that all groups parent macroses are in DB (not removed)"""
    avail_macroses = {x.name for x in CURDB.hbfmacroses.get_hbf_macroses() if (not x.removed)} | {None}

    failed_groups = [x for x in CURDB.groups.get_groups() if x.card.properties.hbf_parent_macros not in avail_macroses]

    if len(failed_groups):
        msg = 'Groups with bad parent macroses: {}'.format(', '.join('group {} with macros {}'.format(x.card.name, x.card.properties.hbf_parent_macros) for x in failed_groups))
        return False, msg
    else:
        return True, None


def check_macros_unknown_parent_macros():
    """Check that all macroses have existing parent macros"""
    avail_macroses = {x.name for x in CURDB.hbfmacroses.get_hbf_macroses() if (not x.removed)}

    failed_macroses = [x for x in CURDB.hbfmacroses.get_hbf_macroses() if (x.parent_macros is not None) and (x.parent_macros not in avail_macroses)]

    if len(failed_macroses):
        msg = 'Macroses with missing parent macros: {}'.format(','.join('macros {} with parent {}'.format(x.name, x.parent_macros) for x in failed_macroses))
        return False, msg

    return True, None


def check_no_invalid_names():
    """Some names are forbidden (like auto-generated groups macroses)"""
    min_project_id = CURDB.hbfranges.min_project_id()
    max_project_id = CURDB.hbfranges.max_project_id()
    forbidden_names = {hbf_group_macro_name(x) for x in CURDB.groups.get_groups()}
    used_names = {x.name for x in CURDB.hbfmacroses.get_hbf_macroses() if ((x.hbf_project_id is None) or (x.hbf_project_id < min_project_id) or (x.hbf_project_id > max_project_id)) and not x.group_macro}

    invalid_names = forbidden_names & used_names
    if invalid_names:
        msg = []
        for invalid_name in invalid_names:
            msg.append('Macros <{}> has same name as macros for group <{}>'.format(invalid_name,  re.match(PATTERN, invalid_name).group(1)))
        msg = '\n'.join(msg)
        return False, msg

    return True, None


def check_no_cycles():
    """Check for no cycles in macroses"""
    for macros in CURDB.hbfmacroses.get_hbf_macroses():
        traversed = {macros.name}
        while True:
            if macros.parent_macros is None:
                break
            elif macros.parent_macros in traversed:
                msg = 'Macroses <{}> forms cycle'.format(' '.join(x for x in traversed))
                return False, msg
            else:
                macros = CURDB.hbfmacroses.get_hbf_macros(macros.parent_macros)
                traversed.add(macros.name)

    return True, None


def check_macros_names():
    """Check if macro name are valid"""
    failed_macroses = [x for x in CURDB.hbfmacroses.get_hbf_macroses() if (not x.external) and (not re.match(PATTERN, x.name))]

    if failed_macroses:
        msg = 'Macroses {} does not satisfy regex <{}>'.format(', '.join('<{}>'.format(x.name) for x in failed_macroses), PATTERN)
        return False, msg

    return True, None


def check_macroses_owners():
    """Check for common owners of gencfg macroses with parent macroses"""
    failed_macroses = []
    for macros in CURDB.hbfmacroses.get_hbf_macroses():
        if (macros.parent_macros is None) or (macros.external) or (macros.skip_owners_check):
            continue

        parent_macros = CURDB.hbfmacroses.get_hbf_macros(macros.parent_macros)
        if parent_macros.name == '_GENCFG_SEARCHPRODNETS_ROOT_':
            continue

        if not len(set(parent_macros.resolved_owners) & set(macros.resolved_owners)):
            failed_macroses.append(macros)

    if failed_macroses:
        msg = []
        for macros in failed_macroses:
            parent_macros = CURDB.hbfmacroses.get_hbf_macros(macros.parent_macros)
            msg.append('Macros {} and parent macros {} have disjont set of owners: <{}> and <{}> respectively'.format(
                macros.name, parent_macros.name, ' '.join(macros.resolved_owners), ' '.join(parent_macros.resolved_owners)))
        msg = '\n'.join(msg)
        return False, msg

    return True, None


def check_macroses_and_groups_owners():
    """Check for common owners of gencfg groups with parent macroses"""

    def _skip_macros(macros_name):
        return macros_name in {
            None,
            '_GENCFG_SEARCHPRODNETS_ROOT_',
        }

    def _skip_groups(group_name):
	# RX-943
        return group_name in {
            'VLA_TSUM_DEV_59EBA9A94BB26888C7DC280C',
            'VLA_TSUM_DEV_59EBA9A94BB26888C7DC280D',
            'VLA_SO_CLUSTERIZATION',
            'SAS_MAPS_CORE_PERSPOI_RENDERER_LOAD',
            'VLA_NGINX_BAMBOOZLED_TEST',
            'VLA_TSUM_TESTING_59F71A1892AD4E7780304F31',
            'VLA_TSUM_LOCAL_5AD9F855F4D088586531CBA6',
            'VLA_TSUM_DEV_5A16D0274BB268B10B21C117',
            'VLA_TSUM_LOCAL_5B3DFED2002DE0B44751E37B',
            'VLA_TSUM_LOCAL_5AE21FE9F4D088459CF198C5',
            'VLA_TSUM_LOCAL_5B053A6F074B6A46DBD07314',
            'VLA_TSUM_TESTING_59F71F79EBF156B038EE24FF',
            'VLA_TSUM_LOCAL_5A325E6B398EC42A245B99E5',
            'VLA_TSUM_TESTING_59F7262E49ED5D2990940F57',
            'SAS_MAPS_CORE_NMAPS_MRC_ONFOOT_MOBILE_STABLE',
            'VLA_TSUM_TESTING_59F71F5CEBF156B038EE24FE',
            'VLA_TSUM_TESTING_5ACE475DF371F6CF300B9330',
            'VLA_TSUM_LOCAL_5B02BECB074B6A03162BCE0E',
            'SAS_COOKIEMATCHER_TEST',
            'VLA_TSUM_LOCAL_ALKEDR_5BB6327D7D7C4E28FFF42966',
            'VLA_TSUM_DEV_5A16EA4A4BB268B10B21C13F',
            'VLA_TSUM_TESTING_5ACE262B0CA3E6AE2D38C807',
            'VLA_TSUM_LOCAL_5B027CC7DC921F5C4E866CDA',
            'VLA_TSUM_DEV_59F5E7EF4BB26857D93D98A0',
            'VLA_TSUM_DEV_5A16F36B4BB268B10B21C143',
            'VLA_TSUM_LOCAL_5AE227ADF4D088088150307A',
            'VLA_TSUM_LOCAL_5A2145EE08BACF348D23398D',
            'VLA_TSUM_TESTING_59F7197692AD4E7780304F30',
            'VLA_TSUM_LOCAL_ALKEDR_5B51D0537A5235695D4A4CEF',
            'VLA_TSUM_LOCAL_5B016450EC6AC4538240F2A5',
            'VLA_TSUM_LOCAL_5A211B3E0294237EB738FF33',
            'VLA_TSUM_DEV_5A16D0274BB268B10B21C118',
            'VLA_TSUM_TESTING_5ACE1D81EBF1568BCDF9D163',
            'VLA_TSUM_TESTING_5ACE1D81EBF1568BCDF9D164',
            'VLA_TSUM_DEV_59F5E7EF4BB26857D93D989F',
            'VLA_TSUM_DEV_5A16EA4B4BB268B10B21C140',
            'VLA_TSUM_DEV_59EBAF3F4BB26888C7DC280F',
            'VLA_CRYPROX_AWACS_TEST',
            'VLA_TSUM_LOCAL_5ADA374DF4D088D15CED0B8F',
            'VLA_TSUM_TESTING_5ACE275FEBF1568BCDF9D16F',
            'VLA_TSUM_TESTING_59F71F5B92AD4E7780304F33',
            'VLA_TSUM_LOCAL_5AD9D0AA4BCF7E4FE929DC74',
            'SAS_ZEN_TEST_STATS',
            'SAS_CRYPROX_TEST',
            'MSK_MYT_BROWSER_DEV_TZEENTCH',
            'VLA_TSUM_TESTING_59F7217FEBF156B038EE2500',
            'VLA_TSUM_LOCAL_5AE21FE9F4D088459CF198C4',
            'VLA_TSUM_LOCAL_5B3E0311002DE0B44751E387',
            'VLA_COOKIEMATCHER_TEST',
            'SAS_CARSHARING_TELEMATICS_TESTING',
            'VLA_TSUM_TESTING_59F71A5092AD4E7780304F32',
            'VLA_TSUM_LOCAL_5ADE3A8A200F808E23CFE205',
            'VLA_TSUM_LOCAL_5B3DFE60002DE0B44751E377',
            'MSK_MYT_SO_CLUSTERIZATION',
            'VLA_MAPS_CORE_NMAPS_MRC_ONFOOT_MOBILE_STABLE',
            'SAS_CRYPROX_AWACS_TEST',
            'MAN_MAPS_CORE_NMAPS_MRC_ONFOOT_MOBILE_STABLE',
            'MAN_CRYPROX_AWACS_TEST',
            'VLA_TSUM_LOCAL_5AE2267DF4D088DE32E166A4',
            'VLA_TSUM_LOCAL_5A184423B35B648ED078FC34',
            'VLA_TSUM_LOCAL_5AE44D7E51EDD61EC5B118AB',
            'VLA_MAPS_CORE_NMAPS_MRC_ONFOOT_PROCESSOR_STABLE',
            'VLA_CARSHARING_TELEMATICS_TESTING',
            'VLA_TSUM_LOCAL_5AE22404F4D088459CF198CC',
            'MAN_CARSHARING_TELEMATICS_TESTING',
            'VLA_TSUM_TESTING_5ACE41AF0CA3E6DC26FF6542',
            'VLA_TSUM_LOCAL_5AFF6CA1AABBE3A7E71C1B2A',
            'VLA_TSUM_LOCAL_5AFF6CA1AABBE3A7E71C1B2B',
            'VLA_TSUM_LOCAL_5AE21565F4D0881919E3C4FD',
            'VLA_TSUM_LOCAL_ALKEDR_5B3362B854049E212E1760F5',
            'VLA_TSUM_LOCAL_5B3E01E3002DE0B44751E383',
            'VLA_CRYPROX_TEST',
            'VLA_TSUM_LOCAL_5ADA358CF4D088D15CED0B88',
            'VLA_TSUM_LOCAL_5B76BDE12B55A42728C8DEA8',
            'VLA_TSUM_LOCAL_5AE2267BF4D088DE32E166A3',
            'SAS_MAPS_CORE_NMAPS_MRC_ONFOOT_PROCESSOR_STABLE',
            'MAN_NGINX_BAMBOOZLED_TEST',
            'SAS_NGINX_BAMBOOZLED_TEST',
            'VLA_TSUM_LOCAL_5B12A034F4D0883F43B0A5EF',
            'VLA_TSUM_TESTING_5ACE41AF0CA3E6DC26FF6543',
            'SAS_SO_CLUSTERIZATION',
            'MAN_CRYPROX_TEST',
            'MAN_MAPS_CORE_NMAPS_MRC_ONFOOT_PROCESSOR_STABLE',
            'MAN_COOKIEMATCHER_TEST',
            #RX-1268
            'MAN_UNIPROXY_DEV',
        }

    failed_groups = []
    for group in CURDB.groups.get_groups():
        if _skip_macros(group.card.properties.hbf_parent_macros) or _skip_groups(group.card.name):
            continue

        parent_macros = CURDB.hbfmacroses.get_hbf_macros(group.card.properties.hbf_parent_macros)

        if parent_macros.group_macro:
            continue

        group_owners = unwrap_dpts(group.card.owners)
        macros_owners = unwrap_dpts(parent_macros.resolved_owners)

        if set(group_owners) and set(macros_owners) and not set(group_owners) & set(macros_owners):
            failed_groups.append(group)

    if failed_groups:
        msg = []
        for group in failed_groups:
            parent_macros = CURDB.hbfmacroses.get_hbf_macros(group.card.properties.hbf_parent_macros)
            msg.append(
                'Group {} and parent macros {} have disjont set of owners: <{}> and <{}> respectively'.format(
                    group.card.name,
                    parent_macros.name,
                    ' '.join(unwrap_dpts(group.card.owners)),
                    ' '.join(parent_macros.resolved_owners)
                )
            )
        msg = '\n'.join(msg)
        return False, msg

    return True, None


def main():
    checkers = [
        check_group_unknown_parent_macros,
        check_macros_unknown_parent_macros,
        check_no_invalid_names,
        check_no_cycles,
        check_macros_names,
        check_macroses_and_groups_owners,
    ]

    status = 0
    for checker in checkers:
        is_ok, msg = checker()
        if not is_ok:
            status = 1
            print '{}:\n{}'.format(checker.__doc__.split('\n')[0], red_text(indent(msg)))

    return status


if __name__ == '__main__':
    status = main()

    sys.exit(status)
