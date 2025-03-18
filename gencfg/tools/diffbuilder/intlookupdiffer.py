"""Differ for intlookup"""

from collections import OrderedDict
import copy

from diffbuilder_types import mkindent, get_field_diff, NoneSomething, EDiffTypes, TDiffEntry


class TIntlookupDiffEntry(TDiffEntry):
    def __init__(self, diff_type, status, watchers, tasks, generated_diff, props=None):
        super(TIntlookupDiffEntry, self).__init__(diff_type, status, watchers, tasks, [], generated_diff, props=props)

    def report_text(self):
        copyd = copy.deepcopy(self.generated_diff)
        pretty_name = "Intlookup %s" % (copyd["intlookupname"])
        copyd.pop("intlookupname")

        d = { 'Intlookups': copyd }

        return self.recurse_report_text(d)


class IntlookupDiffer(object):
    """Diff to compare two intlookups"""

    def __init__(self, oldintlookup, newintlookup, olddb=None, newdb=None):
        assert (oldintlookup is not None or newintlookup is not None)

        self.oldintlookup = oldintlookup
        self.newintlookup = newintlookup
        self.olddb = olddb
        self.newdb = newdb

    def get_diff(self):
        result = OrderedDict()
        result["status"], result["intlookupname"] = self.get_status_diff()

        if result["status"] not in ("ADDED", "REMOVED"):
            props_diff = self.get_content_diff()
            if len(props_diff) > 0:
                result.update(props_diff)
                return TIntlookupDiffEntry(EDiffTypes.INTLOOKUP_DIFF, True, [], [], result)
            else:
                return TIntlookupDiffEntry(EDiffTypes.INTLOOKUP_DIFF, False, [], [], None)
        else:
            return TIntlookupDiffEntry(EDiffTypes.INTLOOKUP_DIFF, True, [], [], result)

    def get_status_diff(self):
        if self.newintlookup is None:
            return "REMOVED", self.oldintlookup.file_name
        elif self.oldintlookup is None:
            return "ADDED", self.newintlookup.file_name
        else:
            return "CHANGED", self.newintlookup.file_name

    def get_content_diff(self):
        result = OrderedDict()

        def map_by_shard(intlookup):
            """Create mapping (tier, shard_id) -> instances"""
            result = dict()

            # found intl2 indexer groups
            intl2_indexes = []
            intl2_index = 0
            for intl2_group in intlookup.intl2_groups:
                intl2_indexes.append(intl2_index)
                intl2_index += len(intl2_group.multishards) * intlookup.hosts_per_group

            # fill table
            for shard_id in xrange(intlookup.get_shards_count()):
                basesearchers = intlookup.get_base_instances_for_shard(shard_id)
                if shard_id % intlookup.hosts_per_group == 0:
                    intsearchers = intlookup.get_int_instances_for_shard(shard_id)
                else:
                    intsearchers = []
                if shard_id in intl2_indexes:
                    intl2searchers = intlookup.get_intl2_instances_for_shard(shard_id)
                else:
                    intl2searchers = []

                index = intlookup.get_tier_and_shard_id_for_shard(shard_id)

                result[index] = dict(intl2searchers=intl2searchers, intsearchers=intsearchers, basesearchers=basesearchers)

            return result

        old_mapping = map_by_shard(self.oldintlookup)
        new_mapping = map_by_shard(self.newintlookup)

        keys = sorted(set(old_mapping.keys() + new_mapping.keys()))
        empty_data = dict(intl2searchers=[], intsearchers=[], basesearchers=[])
        for tier_name, shard_id in keys:
            old_data = old_mapping.get((tier_name, shard_id), empty_data)
            new_data = new_mapping.get((tier_name, shard_id), empty_data)

            for k in ('intl2searchers', 'intsearchers', 'basesearchers'):
                old_instances = sorted([x.full_name() for x in old_data[k]])
                new_instances = sorted([x.full_name() for x in new_data[k]])
                get_field_diff(result, 'Tier {}, shard {}'.format(tier_name, shard_id), old_instances, new_instances)

        return result
