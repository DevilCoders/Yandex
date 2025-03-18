"""
    Various auxiliarily helper functions.
"""

import copy
import os
import re
from collections import defaultdict

from core.db import CURDB
from core.instances import TIntl2Group, TMultishardGroup, TIntGroup, FakeInstance
from core.hosts import FakeHost
from gaux.aux_decorators import memoize
import gaux.aux_portovm
import gaux.aux_hbf


def split_include_name_into_elems(s):
    """
        This function split include string into elements. E. g.:
            - <gaga.gygy.gugu> -> [gaga, gygy, gugu]
            - <gaga[anchor1].gygy.gugu[anchor2] -> [gaga[anchor1], gygy, gugu[anchor2]]
            - <gaga[complex.anchor].gygy[another.complex.anchor] -> [gaga[complex.anchor], gygy[another.complex.anchor]]

        :param s(str): input sting to be divided into parts
        :return (list of str): divided string as list of its parts
    """

    class EStates:
        OUTBRACES = 0
        INBRACES = 1

    elems = []
    cur_elem = []
    state = EStates.OUTBRACES
    for i, c in enumerate(s):
        if state == EStates.INBRACES:
            if c == "[":
                raise Exception("Can not split <%s> into elem (bad symbol at position %d)" % (s, i))
            elif c == "]":
                state = EStates.OUTBRACES
                cur_elem.append(c)
            else:
                cur_elem.append(c)
        elif state == EStates.OUTBRACES:
            if c == "[":
                state = EStates.INBRACES
                cur_elem.append(c)
            elif c == "]":
                raise Exception("Can not split <%s> into elem (bad symbol at position %d)" % (s, i))
            elif c == ".":
                elems.append("".join(cur_elem))
                cur_elem = []
            else:
                cur_elem.append(c)

    assert len(cur_elem) > 0
    assert state == EStates.OUTBRACES

    elems.append("".join(cur_elem))

    return elems


def create_include_path_from_string(s, fname=None):
    """
        Function creates include "path" object from string representation. E. g.
            - configs[gagaga].something.something2[gygygy] -> [(fname, None), (configs, gagaga), (something, None), (something2, gygygy)]
            - somefile.yaml:something.something2 -> [(somefile.yaml, None), (something, None), (something, None)]

        :param s(str): path containing string
        :param fname(str): path fo filename (some path do not contain file name so we should add "fname")

        :return (list of (str, str or None)): path object
    """

    if s.find(':') > 0:
        result = [(os.path.realpath(s.partition(':')[0]), None)]
        s = s.partition(':')[2]
    else:
        if fname is None:
            raise Exception("String <%s> does not contain filename" % s)
        result = [(os.path.realpath(fname), None)]

    for elem in split_include_name_into_elems(s):
        m = re.match('^(\w+)\[([\w\-\._]+)]$', elem)
        if m:
            result.append((m.group(1), m.group(2)))
            continue

        m = re.match('^(\w+)$', elem)
        if m:
            result.append((m.group(1), None))
            continue

        raise Exception("Can not parse <%s> as include path (unparsed element <%s>)" % (s, elem))

    return result


def is_subpath(mypath, otherpath):
    """
        Function checks if <mypath> is subpath of <otherpath>, i. e. their common prefix is equal to otherpath
    """
    return mypath[:len(otherpath)] == otherpath


def convert_yaml_dict_to_list(lst, path):
    """
        We want to have list of pairs (key, value). But from yaml we load something like this [{key1 : value}, {key2, value2}, ...].
        Thus we better convert to list of pairs.

        :param lst(list): initial list with dict
        :param path(list of (str, str)): path to node
        :return (list of (str, something)): pairs converted from list of dicts
    """
    result = []
    for d in lst:
        if len(d) != 1:
            raise Exception("Found dict with <%s> keys (should be exactly one) at path <%s>" % (",".join(d.keys()), path))
        result.append(next(d.iteritems()))
    return result


def convert_yaml_dict_to_list_recurse(data, path):
    """
        We want to have list of pairs (key, value). But from yaml we load something like this [{key1 : value}, {key2 : value2}, ...].
        Thus we should convert this dicts in more readable thing recursively.
    """

    if isinstance(data, (bool, int, float, str)) or data is None:
        return data
    elif isinstance(data, list):
        result = []

        anchor = None
        for d in data:
            if isinstance(d, dict) and '_a' in d:
                anchor = d['_a']

        for d in data:
            if not isinstance(d, dict):
                raise Exception("Convertion yaml dict to list at path <%s> failed: found non-dict type <%s> with value <%s>" % (path, type(d), d))
            elif len(d) != 1:
                raise Exception("Convertion yaml dict to list at path <%s> failed: found dict with keys <%s> (should be exactly one)" % (path, ",".join(d.keys())))

            k, v = next(d.iteritems())
            path.append((k, anchor))
            result.append((k, convert_yaml_dict_to_list_recurse(v, path)))
            path.pop()

        return result
    else:
        raise Exception("Convertion yaml dict to list at path <%s> failed: found non-list type <%s> with value <%s>" % (path, type(data), data))


def _load_group_or_intlookup(s):
    """
        Return intlokup created from string with intlookup or group name
    """

    if CURDB.intlookups.has_intlookup(s):
        return CURDB.intlookups.get_private_intlookup_copy(s)
    elif CURDB.groups.has_group(s):
        group = CURDB.groups.get_group(s)

        intlookup = CURDB.intlookups.create_tmp_intlookup()
        intlookup.hosts_per_group = 1
        intlookup.brigade_groups_count = 1
        intlookup.base_type = group.card.name
        intlookup.file_name = s

        if group.card.searcherlookup_postactions.custom_tier.enabled:
            intlookup.tiers = [group.card.searcherlookup_postactions.custom_tier.tier_name]
        else:
            intlookup.tiers = None

        multishard_group = TMultishardGroup()
        all_instances = group.get_instances()
        all_instances.sort(key=lambda x: x.short_name())
        multishard_group.brigades = map(lambda x: TIntGroup([[x]], []), all_instances)

        intl2_group = TIntl2Group([multishard_group])
        intlookup.intl2_groups.append(intl2_group)

        return intlookup
    else:
        raise Exception("Can not parse <%s> as intlookup or group name" % (s))


def transform_intlookup(intlookup, s):
    """Transform intlookup by restrict it to specified tier or replace instances by by instances of another group

    Transform string is comma-separated list of transformation:
        - 'tier_name' - reduce intlookup to specified tier
        - 'group1->group2' - replace all instances of group1 in intlookup by instances of group2"""

    elems = s.split(',')

    for elem in elems:
        if elem.find('->') > 0:  # found replacing instances by another group instances
            old_group_name, _, new_group_name = elem.partition('->')

            if not CURDB.groups.has_group(old_group_name):
                raise Exception('Group <{}> not found in db while transforming <{}>'.format(old_group_name, intlookup.file_name))
            old_group = CURDB.groups.get_group(old_group_name)
            if not CURDB.groups.has_group(new_group_name):
                raise Exception('Group <{}> not found in db while transforming <{}>'.format(new_group_name, intlookup.file_name))
            new_group = CURDB.groups.get_group(new_group_name)

            # check that we have instances of old group
            used_groups = {x.type for x in intlookup.get_used_instances()}
            if old_group.card.name not in used_groups:
                raise Exception('Not found instances of type <{}> in <{}>'.format(old_group.card.name, intlookup.file_name))

            def replace_instance_func(instance):
                if instance.type == old_group.card.name:
                    new_instance = CURDB.groups.get_instance_by_N(instance.host.name, new_group.card.name, instance.N)
                    return new_instance

            intlookup.replace_instances(replace_instance_func, run_base=True, run_int=True, run_intl2=True)
        else:  # shrink to tier
            intlookup.intl2_groups = intlookup.shrink_intl2groups_for_tier(elem)
            intlookup.tiers = [elem]


@memoize
def calculate_intlookup_expr(s, path=None):
    """
        Having simple expression on intlookups, we generate result intlookup as "sum" of all intlookups in expression. If specified group without inloookup,
        create sample one. Examples:
            - backends:<hostname1>:<port1>:<power1>,...<hostnameN>:<portN>:<powerN> - list of raw backends
            - intlookup1[Tier]+...intlookupN[Tier] - Intlookups, joined by shard. If tier is specified, get only instances with this tier otherwise get all instances
            - group1 - Group name
    """

    if s == "":
        raise Exception("Got empty string as intlookups expression")

    try:
        if s.startswith("backends:"):  # List of backends in format <hostname>:<port>:<power>,...
            intlookup = CURDB.intlookups.create_tmp_intlookup()
            intlookup.hosts_per_group = 1
            intlookup.brigade_groups_count = 1
            intlookup.base_type = "UNKNOWN"
            intlookup.file_name = s
            intlookup.tiers = None

            multishard_group = TMultishardGroup()
            instances_str = s.partition(":")[2].split(",")
            for instance_str in instances_str:
                m = re.match("(^[a-zA-Z][\w\.-]*):(\d+):(\d+)$", instance_str)
                if not m:
                    raise Exception, "Can not parse backend <%s> in form <somehost:port:power> , e. g. <news.yandex.ru:12345:12>" % instance_str
                hostname = m.group(1)
                port = int(m.group(2))
                power = float(m.group(3))
                fake_instance = FakeInstance(FakeHost(hostname), port, power=power)
                multishard_group.brigades.append(TIntGroup([[fake_instance]], []))

            intl2_group = TIntl2Group([multishard_group])
            intlookup.intl2_groups.append(intl2_group)

            return intlookup
        else:
            entries = s.split("+")
            intlookups = []
            for entry in entries:
                if entry.find('[') > 0:
                    transform_data = re.match('^.*\[(.*)\]$', entry).group(1)
                    entry = entry.partition('[')[0]
                else:
                    transform_data = None

                intlookup = _load_group_or_intlookup(entry)
                if transform_data is not None:
                    transform_intlookup(intlookup, transform_data)

                intlookups.append(intlookup)

            # ====================================== RX-266 START ==============================================
            # modify power of instances based on <configs.intmetasearch.config_power> value
            instances_by_group = defaultdict(list)
            for intlookup in intlookups:
                for instance in intlookup.get_used_instances():
                    instances_by_group[instance.type].append(instance)

            for group_name, instances in instances_by_group.iteritems():
                group = CURDB.groups.get_group(group_name)
                if group.card.configs.intmetasearch.config_power is None:
                    continue
                for instance in instances:
                    instance.power = group.card.configs.intmetasearch.config_power

            for intlookup in intlookups:
                for int_group in intlookup.get_int_groups():
                    int_group.power = min(map(lambda x: sum(map(lambda y: y.power, x)), int_group.basesearchers))
            # ====================================== RX-266 FINISH =============================================

            if len(intlookups) == 1:
                return intlookups[0]
            else:
                # check if intlookups match each other
                tiers_list = set(map(lambda x: tuple(x.tiers) if isinstance(x.tiers, list) else None, intlookups))
                assert len(tiers_list) == 1, "Intlookups have more than one unique tiers list: %s" % tiers_list
                assert len(set(map(lambda x: x.hosts_per_group,
                                   intlookups))) == 1, "Intlookups have different values hosts_per_group"
                assert len(set(map(lambda x: len(x.intl2_groups),
                                   intlookups))) == 1, "Intlookups have different number of intl2 groups"

                # now join all intlookups by instances
                result_intlookup = CURDB.intlookups.create_tmp_intlookup()
                result_intlookup.brigade_groups_count = len(intlookups[0].get_multishards())
                result_intlookup.hosts_per_group = intlookups[0].hosts_per_group
                result_intlookup.file_name = "+".join(map(lambda x: x.file_name, intlookups))
                result_intlookup.tiers = intlookups[0].tiers
                result_intlookup.intl2_groups = map(lambda x: TIntl2Group(), xrange(len(intlookups[0].intl2_groups)))

                for intl2_group_id, intl2_group in enumerate(result_intlookup.intl2_groups):
                    intl2_group.intl2searchers = sum(map(lambda x: x.intl2_groups[intl2_group_id].intl2searchers, intlookups), [])
                    intl2_group.multishards = map(lambda x: TMultishardGroup(),
                                                  xrange(len(intlookups[0].intl2_groups[intl2_group_id].multishards)))
                    for intlookup_id, intlookup in enumerate(intlookups):
                        for brigade_group_id, brigade_group in enumerate(
                                intlookup.intl2_groups[intl2_group_id].multishards):
                            for brigade in brigade_group.brigades:
                                # if not self.preserve_2nd_config_ints and intlookup_id > 0:
                                #    brigade.intsearchers = []
                                result_intlookup.intl2_groups[intl2_group_id].multishards[
                                    brigade_group_id].brigades.append(brigade)
                return result_intlookup
    except Exception, e:
        raise Exception("Can not parse as intlookup expression <%s> at path <%s>: error <%s>" % (s, path, str(e)))


def generate_guest_instance_hostname(instance):
    """Generate hostname for guest instance (RX-398)"""
    group = CURDB.groups.get_group(instance.type)
    if group.card.properties.mtn.use_mtn_in_config:
        return gaux.aux_hbf.generate_mtn_hostname(instance, group, '')
    elif group.has_portovm_guest_group():
        return gaux.aux_portovm.gen_vm_host_name(CURDB, instance)
    else:
        return None


def generate_guest_instance_addr(instance):
    """Generate ipv6addr for guest instance (RX-398)"""
    group = CURDB.groups.get_group(instance.type)
    if group.card.properties.mtn.use_mtn_in_config or (group.card.properties.mtn.portovm_mtn_addrs and ('vlan688' in instance.host.vlans)):
        return gaux.aux_hbf.generate_mtn_addr(instance, group, 'vlan688')
    else:
        return None


def may_be_guest_instance(instance, custom_port=None):
    """Check if instance belongs to portovm groups and replace it with 'guest' instance (RX-141)"""
    if instance.type == 'FAKE':
        return instance

    guest_instance_hostname = generate_guest_instance_hostname(instance)
    if guest_instance_hostname is None:
        return instance

    group = CURDB.groups.get_group(instance.type)
    fake_host = FakeHost(guest_instance_hostname)
    fake_host.switch = instance.host.switch
    fake_host.queue = instance.host.queue
    fake_host.dc = instance.host.dc
    fake_host.vlans = copy.copy(instance.host.vlans)

    port = custom_port or instance.port
    fake_instance = FakeInstance(fake_host, port, instance.host.power)
    fake_instance.type = '{}_GUEST'.format(instance.type)

    # ========================= GENCFG-1360 START ========================================
    guest_instance_addr = generate_guest_instance_addr(instance)
    if guest_instance_addr is not None:
         fake_instance.hbf_mtn_addr = guest_instance_addr
    # ========================= GENCFG-1360 FINISH =======================================

    return fake_instance
