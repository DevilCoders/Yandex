from collections import OrderedDict
import copy

from diffbuilder_types import mkindent, get_field_diff, NoneSomething, EDiffTypes, TDiffEntry


class TTierDiffEntry(TDiffEntry):
    def __init__(self, diff_type, status, watchers, tasks, generated_diff, props=None):
        super(TTierDiffEntry, self).__init__(diff_type, status, watchers, tasks, [], generated_diff, props=props)

    def report_text(self):
        copyd = copy.deepcopy(self.generated_diff)
        pretty_name = "Tier %s" % (copyd["tiername"])
        copyd.pop("tiername")

        d = {
            pretty_name: copyd,
        }

        return self.recurse_report_text(d)


class TierDiffer(object):
    def __init__(self, oldtier, newtier, olddb=None, newdb=None):
        assert (oldtier is not None or newtier is not None)

        none_params = {
            'get_shards_count': lambda: 0,
            'get_primuses': lambda: [],
            'shard_ids': [],
            'properties': NoneSomething({'min_replicas': 0}),
        }

        self.oldtier = oldtier if oldtier else NoneSomething(none_params)
        self.newtier = newtier if newtier else NoneSomething(none_params)
        self.olddb = olddb
        self.newdb = newdb

    def get_diff(self):
        result = OrderedDict()
        result["status"], result["tiername"] = self.get_status_diff()

        props_diff = self.get_content_diff()
        if len(props_diff) > 0:
            result.update(props_diff)
            return TTierDiffEntry(EDiffTypes.TIER_DIFF, True, [], [], result)
        else:
            return TTierDiffEntry(EDiffTypes.TIER_DIFF, False, [], [], None)

    def get_status_diff(self):
        if isinstance(self.newtier, NoneSomething):
            return "REMOVED", self.oldtier.name
        elif isinstance(self.oldtier, NoneSomething):
            return "ADDED", self.newtier.name
        else:
            return "CHANGED", self.newtier.name

    def get_content_diff(self):
        result = OrderedDict()
        get_field_diff(result, "Shards count", self.oldtier.get_shards_count(), self.newtier.get_shards_count())
        get_field_diff(result, "Primuses list", self.oldtier.get_primuses(), self.newtier.get_primuses())
        get_field_diff(result, "Shards", self.oldtier.shard_ids, self.newtier.shard_ids)

        if hasattr(self.oldtier, 'properties') and hasattr(self.newtier, 'properties'):
            get_field_diff(result, "MinReplicas", self.oldtier.properties['min_replicas'], self.newtier.properties['min_replicas'])

        return result
