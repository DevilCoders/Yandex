from diffbuilder_types import mkindent, EDiffTypes, TDiffEntry


class SearcherlookupDiffer(object):
    """
        This class is DEPRECATED and does not work with current code
    """

    def __init__(self, olddb, newdb):
        self.olddb = olddb
        self.newdb = newdb

    def load_tags(self, db):
        result = set()
        for group in db.groups.get_groups():
            if len(group.intlookups) == 0:
                for instance in group.get_instances():
                    for prj in group.tags.prj:
                        result.add((group.tags.ctype, group.tags.itype, prj, instance.host.location, 'none'))
            else:
                for intlookup in map(lambda x: db.intlookups.get_intlookup(x), group.intlookups):
                    for instances, tier in intlookup.get_used_instances_with_tier():
                        for instance in instances:
                            for prj in group.tags.prj:
                                result.add((group.tags.ctype, group.tags.itype, prj, instance.host.location, tier))

        return result

    def get_golovan_tags_diff(self):
        GOLOVAN_TAGS_OWNERS = ['khamukhin', 'bkht', 'nobus']

        # Currently instance tags in searcherlookup can differ from same tags in group cards
        # So using groups to get tags is not precise

        olddb_tags = self.load_tags(self.olddb)
        newdb_tags = self.load_tags(self.newdb)

        added_tags = newdb_tags - olddb_tags
        removed_tags = olddb_tags - newdb_tags

        result = []

        if len(added_tags) > 0:
            body = '\n'.join(map(lambda x: '     a_ctype_%s,a_itype_%s,a_prj_%s,a_geo_%s,a_tier_%s' % x, added_tags))
            result.append(
                TDiffEntry(EDiffTypes.SEARCHERLOOKUP_DIFF, True, GOLOVAN_TAGS_OWNERS, [], [], 'Added tags:\n%s' % body))
        if len(removed_tags) > 0:
            body = '\n'.join(map(lambda x: '     a_ctype_%s,a_itype_%s,a_prj_%s,a_geo_%s,a_tier_%s' % x, removed_tags))
            result.append(
                TDiffEntry(EDiffTypes.SEARCHERLOOKUP_DIFF, True, GOLOVAN_TAGS_OWNERS, [], [], 'Removed tags:\n%s' % body))

        return result

    def get_diff(self):
        result = []

        result.extend(self.get_golovan_tags_diff())

        return result
