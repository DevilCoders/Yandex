# cython: profile=True, socheck=True, boundscheck=False, wraparound=False, initializedcheck=False, cdivision=True, language_level=2, infer_types=True, c_string_type=bytes

import os
import tempfile
import time
import copy
import cStringIO
from collections import defaultdict

try:  # to work with skynet
    import ujson
except ImportError:
    pass

from gaux.aux_colortext import red_text
from core.settings import SETTINGS

from core.instances cimport Instance, TIntGroup, TMultishardGroup, TIntl2Group
from core.ghi cimport GHI

cdef class Intlookup:
    cdef public object db
    cdef GHI ghi
    cdef public bint temporary
    # content of intlookup
    cdef public list intl2_groups
    # intlookup properties
    cdef public list tiers
    cdef public bytes file_name
    cdef public int brigade_groups_count
    cdef public int hosts_per_group
    cdef public bytes base_type
    # flags not saved into intlookups
    cdef public bint modified

    def __cinit__(self, object db, bytes fname=None):
        self.db = db
        self.ghi = db.groups.ghi

        self.intl2_groups = []
        self.temporary = False
        if fname is not None:
            if self.db.version <= '2.2.9':
                self.load_intlookup_from_file(fname)
            else:
                self.load_intlookup_from_file_json(fname)

        self.modified = False

    cdef void load_intlookup_from_file(self, bytes filename):
        raise NotImplementedError("Method <load_intlookup_from_file> not implemented")

    cdef inline Instance get_instance_from_intlookup(self, bytes s):
        """
            Convert line of <host:port:power:groupname> format into real instance
        """
        cdef bytes host
        cdef bytes port
        cdef bytes power
        cdef bytes groupname

        host, port, power, groupname = s.split(':')
        return self.ghi.get_instance(groupname, host, int(port), float(power))


    cdef TIntl2Group load_multishards_from_json(self, list content, bint version_2_2_18):
        """
            Load multishard infro from content - list of multishards (every item is multishard)
        """

        cdef TIntl2Group intl2_group = TIntl2Group()
        cdef TMultishardGroup multishard_group
        cdef TIntGroup int_group
        cdef int group_row_id
        cdef list intsearchers
        cdef list basesearchers
        cdef list brigade_row

        for group_row_id, group_row in enumerate(content):
            if group_row_id == 0 and not version_2_2_18:  # in new intlookups with intl2 first row is array of intsearchers
                intl2_group.intl2searchers = map(lambda x: self.get_instance_from_intlookup(bytes(x)), group_row)
                continue

            multishard_group = TMultishardGroup()
            for brigade_row in group_row:
                intsearchers = [self.get_instance_from_intlookup(bytes(x)) for x in brigade_row[1]]
                basesearchers = [[self.get_instance_from_intlookup(bytes(y)) for y in x] for x in brigade_row[2:]]

                if len(basesearchers) != self.hosts_per_group:
                    raise Exception("Need %d basesearchers in group, when have %d; first basesearch %s:%s" % \
                                     (self.hosts_per_group, len(basesearchers), basesearchers[0][0].host.name,
                                      basesearchers[0][0].port))

                int_group = TIntGroup(basesearchers, intsearchers)
                multishard_group.brigades.append(int_group)

            intl2_group.multishards.append(multishard_group)

        return intl2_group

    cdef void load_intlookup_from_file_json(self, bytes filename) except *:
        try:
            content = ujson.loads(open(filename).read())
        except Exception, e:
            raise Exception("Can not load file <%s> as json. Error: %s" % (filename, str(e)))

        if content[0]["tiers"] is not None:
            self.tiers = map(lambda x: bytes(x), content[0]["tiers"])
        else:
            self.tiers = None
        self.brigade_groups_count = content[0]["brigade_groups_count"]
        self.file_name = <bytes>(content[0]["file_name"])
        self.hosts_per_group = content[0]["hosts_per_group"]
        self.base_type = <bytes>(content[0]["base_type"])

        if self.tiers is not None:
            self.brigade_groups_count = sum(self.db.tiers.get_total_shards(x) for x in self.tiers) / self.hosts_per_group
        self.file_name = os.path.basename(filename)

        if self.db.version <= '2.2.18':  # old-style intlookups without Intl2
            if len(content) > 1:  # if len(content) == 1, this means we have no multishard groups in intlookup
                self.intl2_groups.append(self.load_multishards_from_json(content[1:], True))
        else:
            for elem in content[1:]:
                self.intl2_groups.append(self.load_multishards_from_json(elem, False))

        cdef int loaded_groups_count = sum(map(lambda x: len(x.multishards), self.intl2_groups))
        if self.brigade_groups_count != loaded_groups_count:
            raise Exception("Intlookup %s: brigade groups count %d not equal to number of groups %d" % (self.file_name, self.brigade_groups_count, loaded_groups_count))

    cdef void write_intlookup_to_file(self, filename=None):
        raise NotImplementedError("Method <write_intlookup_to_file> not implemented")

    def write_multishards_to_stream(self, list multishard_groups, bytes start_indent, object stream):
        cdef int multishard_index
        cdef TMultishardGroup multishard_group
        cdef int int_index
        cdef TIntGroup int_group
        cdef int host_list_index
        cdef list host_list

        for multishard_index, multishard_group in enumerate(multishard_groups):
            print >> stream, (start_indent + '[')

            for int_index, int_group in enumerate(multishard_group.brigades):
                print >> stream, (start_indent + SETTINGS.core.text.indent + '[%s,' % ujson.dumps(dict(map(lambda x: (x, getattr(int_group, x)), ['power', ]))))
                print >> stream, (start_indent + SETTINGS.core.text.indent + '[%s],' % ", ".join(map(lambda x: '"%s:%s:%s:%s"' % (x.host.name, x.port, x.power, x.type), int_group.intsearchers)))

                for host_list_index, host_list in enumerate(int_group.basesearchers):
                    if host_list_index == len(int_group.basesearchers) - 1:
                        postfix = ""
                    else:
                        postfix = ","

                    ss = start_indent + SETTINGS.core.text.indent * 2 + "[%s]%s" % (", ".join(map(lambda x: '"%s:%s:%.3f:%s"' % (x.host.name, x.port, x.power, x.type), host_list)), postfix)
                    print >> stream, ss

                if int_index == len(multishard_group.brigades) - 1:
                    print >> stream, (start_indent + SETTINGS.core.text.indent + ']')
                else:
                    print >> stream, (start_indent + SETTINGS.core.text.indent + '],')

            if multishard_index == len(multishard_groups) - 1:
                print >> stream, (start_indent + ']')
            else:
                print >> stream, (start_indent + '],')

    def write_intlookup_to_file_json(self, filename=None):
        cdef TIntGroup int_group
        cdef object stream = cStringIO.StringIO()
        cdef int intl2_group_id
        cdef TIntl2Group intl2_group

        indent = SETTINGS.core.text.indent

        # update brigades
        for int_group in self.get_int_groups():
            int_group.reinit()

        print >> stream, ('[')
        options_dict = {
            'file_name': self.file_name,
            'hosts_per_group': self.hosts_per_group,
            'brigade_groups_count': self.brigade_groups_count,
            'base_type': self.base_type,
            'tiers': self.tiers,
        }
        if len(self.intl2_groups) == 0 or (len(self.intl2_groups) == 1 and len(self.intl2_groups[0].multishards) == 0):
            print >> stream, ujson.dumps(options_dict)
        else:
            print >> stream, ujson.dumps(options_dict) + ","

        if self.db.version <= '2.2.18':
            self.write_multishards_to_stream(self.get_multishards(), indent, stream)
        else:
            for intl2_group_id, intl2_group in enumerate(self.intl2_groups):
                print >> stream, indent + '['

                # write intl2 searchers
                print >> stream, (indent * 2 + '[%s],' % ", ".join(map(lambda x: '"%s:%s:%.3f:%s"' % (x.host.name, x.port, x.power, x.type), intl2_group.intl2searchers)))

                self.write_multishards_to_stream(intl2_group.multishards, indent * 2, stream)

                if intl2_group_id == len(self.intl2_groups) - 1:
                    print >> stream, indent + ']'
                else:
                    print >> stream, indent + '],'

        print >> stream, (']')

        if filename:
            f = open(filename, 'w', buffering=2 * 1024 * 1024)
        else:
            f = open(os.path.join(self.db.INTLOOKUP_DIR, self.file_name), 'w', buffering=2 * 1024 * 1024)
        f.write(stream.getvalue())
        if filename:
            f.close()

    cpdef mark_as_modified(self):
        self.modified = True

    cpdef reset(self, bint drop_brigade_groups=True):
        if drop_brigade_groups:
            self.brigade_groups_count = 0
            self.intl2_groups = []
            self.tiers = None
        else:
            for brigade_group in self.get_multishards():
                brigade_group.brigades = []

        self.modified = True

    cpdef bint is_empty(self):
        return self.get_shards_count() == 0

    cpdef list get_multishards(self):
        return sum([x.multishards for x in self.intl2_groups], [])

    cpdef list get_int_groups(self):
        cdef list result = []
        for intl2_group in self.intl2_groups:
            for multishard_group in intl2_group.multishards:
                for int_group in multishard_group.brigades:
                    result.append(int_group)
        return result

    cpdef list get_used_base_instances(self):
        cdef TMultishardGroup brigade_group
        cdef TIntGroup brigade
        cdef list basesearch_list

        result = []
        for brigade_group in self.get_multishards():
            for brigade in brigade_group.brigades:
                for basesearch_list in brigade.basesearchers:
                    for x in basesearch_list:
                        result.append(x)
        return result

    cpdef list get_base_instances(self):
        return self.get_used_base_instances()

    cpdef list get_used_int_instances(self):
        cdef TMultishardGroup brigade_group
        cdef TIntGroup brigade

        result = []
        for brigade_group in self.get_multishards():
            for brigade in brigade_group.brigades:
                for intsearch in brigade.intsearchers:
                    result.append(intsearch)
        return result

    cpdef list get_int_instances(self):
        return self.get_used_int_instances()

    cpdef list get_intl2_instances(self):
        return sum([x.intl2searchers for x in self.intl2_groups], [])

    cpdef list get_used_instances(self):
        return self.get_used_base_instances() + self.get_used_int_instances() + self.get_intl2_instances()

    cpdef list get_used_instances_by_shard(self):
        return [self.get_base_instances_for_shard(x) for x in xrange(self.get_shards_count())]

    cpdef get_used_instances_with_tier(self):
        return [(self.get_base_instances_for_shard(x), self.get_tier_for_shard(x)) for x in xrange(self.get_shards_count())]

    cpdef list get_instances(self):
        return self.get_base_instances() + self.get_int_instances() + self.get_intl2_instances()

    cpdef int get_shards_count(self):
        return len(self.get_multishards()) * self.hosts_per_group

    cpdef get_base_instances_for_shard(self, int shard_id):
        if shard_id >= self.hosts_per_group * self.brigade_groups_count:
            raise Exception("Have only %d shards, when %d-th requested (%s)" % (self.hosts_per_group * self.brigade_groups_count, shard_id, self.file_name))
        cdef list multishards = self.get_multishards()
        br = multishards[shard_id / self.hosts_per_group]
        cdef list result = []
        for x in br.brigades:
            result.extend((<TIntGroup>x).basesearchers[shard_id % self.hosts_per_group])
        return result

    cpdef list get_base_instances_for_replica(self, replica_id):
        """
            Return flat list of baseseachers from specified replica. Needed for basesearch config generator for refresh
        """
        cdef list result = []

        for multishard in self.get_multishards():
            if len((<TMultishardGroup>multishard).brigades) <= replica_id:
                continue
            result.extend(sum(multishard.brigades[replica_id].basesearchers, []))

        return result

    cpdef list get_int_instances_for_shard(self, int shard_id):
        if shard_id >= self.hosts_per_group * self.brigade_groups_count:
            raise Exception("Have only %d shards, when %d-th requested (%s)" % (self.hosts_per_group * self.brigade_groups_count, shard_id, self.file_name))
        cdef list multishards = self.get_multishards()
        br = multishards[shard_id / self.hosts_per_group]
        cdef list result = []
        for x in br.brigades:
            result.extend((<TIntGroup>x).intsearchers)
        return result

    cpdef list get_intl2_instances_for_shard(self, int shard_id):
        if shard_id >= self.hosts_per_group * self.brigade_groups_count:
            raise Exception("Have only %d shards, when %d-th requested (%s)" % (self.hosts_per_group * self.brigade_groups_count, shard_id, self.file_name))
        index = 0
        for intl2_group in self.intl2_groups:
            if len(intl2_group.multishards) * self.hosts_per_group + index < shard_id:
                index += len(intl2_group.multishards) * self.hosts_per_group
                continue
            return intl2_group.intl2searchers

    cpdef tuple get_tier_and_shard_id_for_shard(self, int shard_id):
        if self.tiers is None:
            return b'None', shard_id

        cdef int cur_shard = 0
        for tier in self.tiers:
            cur_shard += self.db.tiers.get_total_shards(tier)
            if shard_id < cur_shard:
                return tier, shard_id - cur_shard + self.db.tiers.get_total_shards(tier)
        raise Exception("File <%s>: have only %d shards, when %d-th requested" % (self.file_name, self.hosts_per_group * self.brigade_groups_count, shard_id))

    cpdef bytes get_tier_for_shard(self, shard_id):
        return self.get_tier_and_shard_id_for_shard(shard_id)[0]

    cpdef bytes get_primus_for_shard(self, int shard_id, bint for_searcherlookup=True):
        cdef int cur_shard = 0
        if self.tiers is None:
            return b"None"
        for tier in self.tiers:
            cur_shard += self.db.tiers.get_total_shards(tier)
            if shard_id < cur_shard:
                if for_searcherlookup:
                    return self.db.tiers.primus_shard_id_for_searcherlookup(tier, shard_id - cur_shard)
                else:
                    return self.db.tiers.tiers[tier].get_shard_id(shard_id - cur_shard)
        raise Exception("File <%s>: have only %d shards, when %d-th requested" % (self.file_name, self.hosts_per_group * self.brigade_groups_count, shard_id))

    cpdef list calc_brigade_power(self, list multishards=None, object without_queue=None, object without_dc=None, object without_flt=None,
                                  bint calc_instances=False):
        return self._calc_brigade_power(multishards=multishards, without_queue=without_queue, without_dc=without_dc, without_flt=without_flt, calc_instances=calc_instances)

    cdef list _calc_brigade_power(self, list multishards=None, object without_queue=None, object without_dc=None, object without_flt=None,
                                  bint calc_instances=False):
        if len([x for x in [without_queue, without_dc, without_flt] if x is None]) < 1:
            raise Exception("Called calc brigade_power with without_queue = %s, without_dc = %s" % (without_queue, without_dc))

        cdef bint without_nothing = (len([x for x in [without_queue, without_dc, without_flt] if x is None]) == 3)
        cdef list brigade_powers = []
        cdef TMultishardGroup brigade_group
        cdef TIntGroup brigade

        if multishards is None:
            multishards = self.get_multishards()
        for i in xrange(0, len(multishards)):
            brigade_group = multishards[i]
            brigade_group_power = 0.
            for brigade in brigade_group.brigades:
                power = min(sum((y.power if not calc_instances else 1.) for y in x) for x in brigade.basesearchers)
                if without_queue and brigade.basesearchers[0][0].host.queue != without_queue:
                    brigade_group_power += power
                if without_dc and brigade.basesearchers[0][0].host.dc != without_dc:
                    brigade_group_power += power
                if without_flt and not without_flt(brigade.basesearchers[0][0]):
                    brigade_group_power += power
                if without_nothing:
                    brigade_group_power += power
            brigade_powers.append(brigade_group_power)

        return brigade_powers

    cpdef list calc_durability(self, bint is_smart=True, bint calc_instances=False):
        cdef dict corrected_power = dict()
        cdef dict instances_to_shard = dict()
        cdef list shard_power = [.0 for _ in range(self.hosts_per_group * len(self.get_multishards()))]
        cdef int bg
        cdef TMultishardGroup brigade_group
        cdef int b
        cdef TIntGroup brigade
        cdef int sh
        cdef list shard_basesearchers

        for bg, brigade_group in enumerate(self.get_multishards()):
            for b, brigade in enumerate(brigade_group.brigades):
                for sh, shard_basesearchers in enumerate(brigade.basesearchers):
                    shard = bg * self.hosts_per_group + sh
                    for basesearcher in shard_basesearchers:
                        corrected_power[basesearcher] = brigade.power if not calc_instances else 1.
                        shard_power[shard] += brigade.power if not calc_instances else 1.
                        instances_to_shard[basesearcher] = shard
        weakest_shard_power = round(min(shard_power))

        fp_sets = [(defaultdict(list), 'dc', 'dc'),
                   (defaultdict(list), 'queue', 'queue'),
                   (defaultdict(list), 'switch', 'switch'),
                   (defaultdict(list), 'name', 'host')]
        for x in self.get_used_base_instances():
            for items, attribute, _ in fp_sets:
                items[x.host.__getattribute__(attribute)].append(x)

        result = []
        cutoff = 8
        for failure_points, attribute, prn_attribute in fp_sets:
            min_power = weakest_shard_power
            min_power_points = []
            for failure_point, failure_instances in failure_points.items():
                affected_shards = dict()
                for instance in failure_instances:
                    shard = instances_to_shard[instance]
                    if shard not in affected_shards:
                        affected_shards[shard] = shard_power[shard] - corrected_power[instance]
                    else:
                        affected_shards[shard] -= corrected_power[instance]
                cur_min_power = round(min(affected_shards.values()))
                if min_power > cur_min_power:
                    min_power = cur_min_power
                    min_power_points = [failure_point]
                elif min_power == cur_min_power:
                    min_power_points.append(failure_point)
            if min_power == weakest_shard_power:
                break
            if is_smart:
                min_power_points = min_power_points if len(min_power_points) < cutoff else (
                    list(min_power_points)[:cutoff - 1] + ['...'])
            result.append((prn_attribute, min_power, min_power_points))
            if is_smart and min_power != 0:
                # for large groups we can have too much calculations for hosts
                # so let's try to stop early
                break
        return result

    cpdef tuple get_tier_shards_range(self, bytes tier):
        assert self.tiers is not None, "Intlookup %s do not have tiers" % self.file_name
        assert tier in self.tiers, "Tier %s is not in intlookup %s" % (tier, self.file_name)

        cdef int first = sum([self.db.tiers.get_total_shards(x) for x in self.tiers[:self.tiers.index(tier)]], 0)
        cdef int last = first + self.db.tiers.get_total_shards(tier)

        # assert first % self.hosts_per_group == 0, "Intlookup %s: tier %s starts from %s, which is not divided by %s" % (
        #     self.file_name, tier, first, self.hosts_per_group)
        # assert last % self.hosts_per_group == 0, "Intlookup %s: tier %s ends with %s, which is not divided by %s" % (
        #     self.file_name, tier, last, self.hosts_per_group)

        return first, last

    cpdef shrink_intl2groups_for_tier(self, object first, object second=None, bint filter_empty=True):
        """
            This function shrink intlookup content depending on arguments. Is specified only first argument, then this argument is tier
            an we should shrink intlookup to shards of this tier. If second is not None, than [first, second) is range of shards

            :type first: str
            :type second: int
            :type filter_empty: bool

            :param first: <tier name> or <first shard> in result intlookup
            :param second: <last shard> in result intlookup
            :param filter_empty: filter empty groups
            :return nothing: list of intl2group with multishards only in specified range
        """

        if second is None:
            first, last = self.get_tier_shards_range(first)
        else:
            first, last = first, second

        assert (first <= last) and (first >= 0) and (
            last <= self.get_shards_count()), "Required range [%s, %s) not in [0, %s)" % (
            first, last, self.get_shards_count())
        assert first % self.hosts_per_group == 0, "First shard index %s is not divided by %s" % (
            first, self.hosts_per_group)
        assert last % self.hosts_per_group == 0, "Last shard index %s is not divided by %s" % (
            last, self.hosts_per_group)

        first_multishard = first / self.hosts_per_group
        last_multishard = last / self.hosts_per_group

        result_intl2_groups = []
        cur_multishard = 0
        for intl2_group in self.intl2_groups:
            first_index = max(first_multishard, cur_multishard)
            last_index = min(cur_multishard + len(intl2_group.multishards), last_multishard)
            if first_index <= last_index:
                result_intl2_groups.append(
                    TIntl2Group(intl2_group.multishards[(first_index - cur_multishard):(last_index - cur_multishard)]))
            else:
                result_intl2_groups.append(TIntl2Group())
            cur_multishard += len(intl2_group.multishards)

        if filter_empty:
            result_intl2_groups = [x for x in result_intl2_groups if len(x.multishards) != 0]

        return result_intl2_groups

    cpdef object for_each(self, object func, bint run_base=True, bint run_int=False, object base_value=0):
        result = base_value
        for brigade_group in self.get_multishards():
            for brigade in brigade_group.brigades:
                if run_int:
                    for intsearch in brigade.intsearchers:
                        r = func(intsearch)
                        try:
                            result += r
                        except:
                            pass
                if run_base:
                    for lst in brigade.basesearchers:
                        for basesearch in lst:
                            r = func(basesearch)
                            try:
                                result += r
                            except:
                                pass

        return result

    cpdef for_each_brigade(self, object func):
        for brigade_group in self.get_multishards():
            for brigade in brigade_group.brigades:
                func(brigade)

    cpdef replace_instances(self, object func, bint run_base=True, bint run_int=False, bint run_intl2=False):
        """
            This method replace instances in intlookups. Function <func> with instance as parameter returns new instance (for replacement)
            or None (if nothing to replace).

            :type func: T
            :type run_base: bool
            :type run_int: bool
            :type run_intl2: bool

            :param func: python function, which suggest replacement for instance
            :param run_base: replace base instances
            :param run_int: replace int instances
            :param run_intl2: replace intl2 instances

            :retuls None: modification of intlookups is performed inplace
        """

        is_affected = False

        if run_intl2:
            for intl2group in self.intl2_groups:
                for pos, intl2search in enumerate(intl2group.intl2searchers):
                    r = func(intl2search)
                    if r is not None:
                        intl2group.intl2searchers[pos] = r
                        is_affected = True

        for brigade_group in self.get_multishards():
            for brigade in brigade_group.brigades:
                if run_int:
                    for pos, intsearch in enumerate(brigade.intsearchers):
                        r = func(intsearch)
                        if r is not None:
                            brigade.intsearchers[pos] = r
                            is_affected = True
                if run_base:
                    for lst in brigade.basesearchers:
                        for pos, basesearch in enumerate(lst):
                            r = func(basesearch)
                            if r is not None:
                                lst[pos] = r
                                is_affected = True

        self.modified = True

        return is_affected

    cpdef filter_brigades(self, object filter_f):
        for brigade_group in self.get_multishards():
            brigade_group.brigades = filter(filter_f, brigade_group.brigades)

    cpdef int shards_count(self):
        return len(self.get_multishards()) * self.hosts_per_group

    cpdef update_base_type(self):
        base_types = set()
        if self.base_type:
            base_types.add(self.base_type)
        for basesearch in self.get_base_instances():
            base_types.add(basesearch.type)
        if len(base_types) != 1:
            self.base_type = None
        else:
            self.base_type = <bytes>(list(base_types)[0])

    cpdef remove_hosts(self, list hosts, object group):
        """
            Remove instances with specified hosts from intlookup.

            :type hosts: list[core.hostsinfo.Host]
            :param hosts: list of host to remove from intlookup
            :type group: core.igroups.IGroup
            :param group: group to remove hosts from


            :return None: inplace modify contents of multishards
        """

        # do nothing if intlookup does not contain any of hosts
        if len(set(hosts) & set([x.host for x in self.get_instances()])) == 0:
            return

        # remove hosts from intl2
        for intl2_group in self.intl2_groups:
            intl2_group.intl2searchers = [x for x in intl2_group.intl2searchers if (x.host not in hosts) or (x.type != group.card.name)]

        # remove hosts from int/base
        for multishard in self.get_multishards():
            new_intgroups = []
            for intgroup in multishard.brigades:
                intgroup.intsearchers = [x for x in intgroup.intsearchers if (x.host not in hosts) or (x.type != group.card.name)]
                new_basesearchers = []
                for index, lst in enumerate(intgroup.basesearchers):
                    new_basesearchers.append([x for x in lst if (x.host not in hosts) or (x.type != group.card.name)])
                intgroup.basesearchers = new_basesearchers
                if len(intgroup.get_all_instances()) > 0:
                    new_intgroups.append(intgroup)
            multishard.brigades = new_intgroups
        self.mark_as_modified()

    cpdef remove_instances(self, remove_instances):
        """
            Remove specified instances. If instance in group with hosts_per_group > 1, remove group.
        """

        for multishard in self.get_multishards():
            new_intgroups = []
            for intgroup in multishard.brigades:
                intgroup.intsearchers = [x for x in intgroup.intsearchers if x not in remove_instances]
                if len(set(intgroup.get_all_instances()) & set(remove_instances)) == 0:
                    new_intgroups.append(intgroup)
            multishard.brigades = new_intgroups

        self.mark_as_modified()


cdef class Intlookups:
    cdef object db
    cdef dict intlookups
    cdef dict links
    cdef set existing_intlookups
    # fastcheck information
    cdef int last_check_time
    cdef object last_check_exception

    def __cinit__(self, object db):
        # self.init_groups()
        self.db = db

        try:
            unversioned_intlookups = self.db.get_repo().get_changed_file_statuses(self.db.INTLOOKUP_DIR)['unversioned']
        except:
            unversioned_intlookups = []
        unversioned_intlookups = filter(lambda x: not x.startswith('.'), unversioned_intlookups)
        unversioned_intlookups = map(lambda x: os.path.join(self.db.INTLOOKUP_DIR, x), unversioned_intlookups)
        if len(unversioned_intlookups) > 0:
            print red_text("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            print red_text("FOUND UNVERSIONED INTLOOKUPS <%s>. Remove them if they are not real intlookups." % ",".join(unversioned_intlookups))
            print red_text("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")

        # temp intlookup filenames starts with '.'
        self.intlookups = {str(name): None for name in os.listdir(self.db.INTLOOKUP_DIR) if not name.startswith('.')}
        self.links = {name: set() for name in self.intlookups}
        self.existing_intlookups = set(self.intlookups)

        for group in self.db.groups.get_groups():
            for intlookup_name in group.card.intlookups:
                if intlookup_name not in self.intlookups:
                    raise Exception('No such intlookup "%s" listed in group %s intlookup list' % \
                                     (intlookup_name, group.card.name))
                self.links[intlookup_name].add(group.card.name)

        self.last_check_time = 0
        self.last_check_exception = None

    cpdef add_build_intlookup(self, Intlookup intlookup):
        if intlookup.file_name in self.intlookups:
            self.disconnect(intlookup.file_name)
        self.intlookups[intlookup.file_name] = intlookup
        self.connect(intlookup.file_name)

    cpdef check_name(self, bytes name):
        if os.path.basename(name) != name:
            raise Exception('Intlookup name "%s" should not contain file path characters' % name)
        if not name:
            raise Exception('Intlookup name is empty')

    cpdef check_existance(self, bytes name, bint exists=True):
        self.check_name(name)
        if exists:
            if name not in self.intlookups:
                raise Exception('Intlookup %s does not exist' % name)
        else:
            if name in self.intlookups:
                raise Exception('Intlookup %s already exists' % name)

    cpdef disconnect(self, object intlookup):
        if isinstance(intlookup, Intlookup):
            intlookup = intlookup.file_name
        for group_name in self.links[intlookup]:
            group = self.db.groups.get_group(group_name)
            if intlookup in group.card.intlookups:
                group.mark_as_modified()
            group.card.intlookups = [x for x in group.card.intlookups if x != intlookup]
        self.links[intlookup] = set()

    cpdef connect(self, bytes filename, list groups=None):
        self._connect(filename, groups)

    cdef _connect(self, bytes filename, list groups=None):
        cdef Intlookup intlookup

        if (<Intlookup>self.intlookups[filename]).temporary:
            self.links[filename] = set()
            return

        if groups is None:
            intlookup = self.get_intlookup(filename)
            groups = list(instance.type for instance in intlookup.get_instances())
            if intlookup.base_type:
                groups.append(intlookup.base_type)
                #        base_type = self.get_intlookup(filename).base_type
                #        if base_type is not None:
                #            if not self.db.groups.has_group(base_type):
                #                raise Exception('In intlookup %s undefined base_type %s' % (filename, base_type))
                #            groups.append(base_type)

        self.links[filename] = set(groups)
        for group in [self.db.groups.get_group(x) for x in set(groups)]:
            group.card.intlookups = sorted(group.card.intlookups + [filename])

    cpdef list get_linked_groups(self, object intlookup):
        if not isinstance(intlookup, str):
            intlookup = intlookup.file_name
        return list(self.links[intlookup])

    cpdef bytes get_intlookup_path(self, object intlookup):
        if isinstance(intlookup, Intlookup):
            intlookup = intlookup.file_name
        return bytes(os.path.join(self.db.INTLOOKUP_DIR, intlookup))

    cpdef list get_intlookup_names(self):
        return self.intlookups.keys()

    cpdef bint has_intlookup(self, bytes filename):
        self.check_name(filename)
        return filename in self.intlookups

    cpdef create_empty_intlookup(self, bytes file_name):
        self.check_name(file_name)
        self.check_existance(file_name, exists=False)

        cdef Intlookup result = Intlookup(self.db, None)
        result.file_name = file_name
        result.mark_as_modified()

        self.intlookups[file_name] = result
        self.links[file_name] = set()

        return result

    cpdef Intlookup get_intlookup(self, bytes filename):
        self.check_name(filename)
        self.check_existance(filename)

        if self.intlookups[filename] is None:
            intlookup = Intlookup(self.db, self.get_intlookup_path(filename))
            self.intlookups[filename] = intlookup
        return self.intlookups[filename]

    cpdef list get_intlookups(self, bint include_backups=True):
        result = []
        for name in self.intlookups.keys():
            if include_backups or name.find('backup') == -1 and name.find('bak') == -1:
                result.append(self.get_intlookup(name))
        return result

    cpdef list get_intlookups_names(self):
        """
            This function is needed for optimization purporses (do not load all intlookups in wbe intlookup list)
        """
        return list(self.intlookups.keys())

    cpdef Intlookup get_private_intlookup_copy(self, bytes filename):
        """Returns a private copy of intlookup. All changes in this copy are discarded"""
        return Intlookup(self.db, self.get_intlookup_path(filename))

    cpdef Intlookup copy_intlookup(self, bytes filename, bytes new_filename, object base_type=-1):
        """Creates a copy of intlookup in different file"""
        self.check_name(new_filename)
        self.check_existance(new_filename, exists=False)
        intlookup = Intlookup(self.db, self.get_intlookup_path(filename))
        intlookup.file_name = new_filename
        if base_type != -1:
            intlookup.base_type = base_type
        self.intlookups[new_filename] = intlookup
        self.connect(new_filename)
        return intlookup

    cpdef Intlookup create_tmp_intlookup(self):
        """Creates temporary intlookup with unique file name starting with dot (hidden file)"""
        cdef bytes file_name = self.create_tmp_intlookup_name()
        return self.create_empty_intlookup(file_name)

    cdef bytes create_tmp_intlookup_name(self):
        os_handle, file_name = tempfile.mkstemp(suffix='.py', prefix='.' + SETTINGS.core.tempfile.prefix, dir=self.db.INTLOOKUP_DIR)
        os.close(os_handle)
        os.unlink(file_name)  # we need unused name, not file
        assert (os.path.basename(file_name).startswith('.'))
        return os.path.basename(file_name)

    cpdef void update_intlookup_links(self, object intlookup):
        self.disconnect(intlookup)
        self.connect(intlookup)

    cpdef void remove_intlookup(self, bytes filename):
        self.check_name(filename)
        self.check_existance(filename)

        self.disconnect(filename)
        del self.intlookups[filename]
        del self.links[filename]

    cpdef void rename_intlookup(self, bytes source, bytes target):
        self.check_name(source)
        self.check_name(target)
        self.check_existance(source)
        self.check_existance(target, exists=False)

        groups = self.get_linked_groups(source)
        self.disconnect(source)

        tmp = self.intlookups[source]
        if tmp is None:
            tmp = self.get_intlookup(source)
        self.intlookups[target] = tmp
        self.links[target] = []
        tmp.file_name = target
        tmp.mark_as_modified()

        del self.intlookups[source]
        del self.links[source]
        self.connect(target, groups)

    cpdef void update(self, bint verbose=True, bint smart=False):
        for intlookup in self.intlookups.values():
            if (smart is True) and (intlookup is not None) and (not intlookup.modified):
                continue

            if intlookup:
                self.disconnect(intlookup.file_name)
                self.connect(intlookup.file_name)
                intlookup.update_base_type()

                if self.db.version <= '2.2.9':
                    intlookup.write_intlookup_to_file()
                else:
                    intlookup.write_intlookup_to_file_json()

        to_add = set(self.intlookups) - set(self.existing_intlookups)

        to_add = [intlookup for intlookup in to_add if
                  not (intlookup in self.intlookups and self.intlookups[intlookup].temporary)]

        to_remove = set(self.existing_intlookups) - set(self.intlookups)
        to_remove = [intlookup for intlookup in to_remove if
                     not (intlookup in self.intlookups and self.intlookups[intlookup].temporary)]

        if to_add or to_remove:
            db_repo = self.db.get_repo(verbose=verbose)
            db_repo.add([self.get_intlookup_path(name) for name in to_add])
            db_repo.rm([self.get_intlookup_path(name) for name in to_remove])

        self.existing_intlookups = set(self.intlookups.keys())

    cpdef void rename_group(self, bytes intlookup, bytes old_name, bytes new_name):
        new_lst = []
        for group in self.links[intlookup]:
            if group == old_name:
                new_lst.append(new_name)
            else:
                new_lst.append(group)
        self.links[intlookup] = new_lst

    # create mapping: instance -> (tier, shard_id)
    cpdef build_shard_mapping(self):
        cdef Intlookup intlookup

        result = {}
        for intlookup in self.get_intlookups():
            if intlookup.tiers is None:
                continue
            cur_id = 0
            for tier in intlookup.tiers:
                for j in range(self.db.tiers.get_tier(tier).get_shards_count()):
                    for instance in intlookup.get_base_instances_for_shard(cur_id):
                        result[instance] = (tier, j)
                    cur_id += 1
        return result

    cpdef fast_check(self, object timeout):
        current_time = time.time()

        if current_time < self.last_check_time:
            self.last_check_time = 0
            self.last_check_exception = None

        # will do real check not more than once in 5 seconds
        need_to_check = (current_time - self.last_check_time > 5)

        if not need_to_check:
            if self.last_check_exception:
                raise copy.copy(self.last_check_exception)
            return

        self.last_check_time = current_time
        try:
            self.__fast_check(timeout)
            self.last_check_exception = None
        except Exception as err:
            self.last_check_exception = copy.copy(err)
            raise err

    cdef __fast_check(self, object timeout):
        # it is better if this function once produce an error
        # all next times it will produce an error
        cdef float start_time = time.time()
        cdef list intlookup_names = sorted(self.get_intlookup_names())
        for intlookup_name in intlookup_names:
            if time.time() - start_time >= timeout:
                break
            self.get_intlookup(intlookup_name)
