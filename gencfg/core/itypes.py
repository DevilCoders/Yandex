"""
    Instance types classes and interface.
"""

import os

from core.card.node import CardNode, Scheme, load_card_node, save_card_node


ITYPE_PATTERN = '^[a-zA-Z0-9]{1,93}(?:[a-zA-Z0-9]{1,7}|_custom)\Z'  # GOLOVANSUPPORT-305


class IType(CardNode):
    def __init__(self, parent, card_content):
        """
            Itype node constructor.

            :param parent(core.itypes.ITypes): container of itypes
            :param card_content(CardNode or dict): card node loaded from yaml-file (dict for older versions)
        """

        super(IType, self).__init__()
        self.parent = parent

        for child in card_content:
            self.__dict__[child] = card_content[child]


class ITypes(object):
    def __init__(self, db):
        self.db = db

        self.itypes = dict()

        if self.db.version <= "2.2.7":  # initialize by getting all itypes from groups
            for itype_name in set(map(lambda x: x.card.tags.itype, self.db.groups.get_groups())):
                self.itypes[itype_name] = IType(self, {'name': itype_name})
        elif self.db.version <= "2.2.16":  # initialize from content of dbconstans
            for itype_name in self.db.constants.ITYPES:
                self.itypes[itype_name] = IType(self, {'name': itype_name})
        else:  # initialize from itypes.yaml
            self.SCHEME_FILE = os.path.join(self.db.SCHEMES_DIR, 'itypes.yaml')
            self.DATA_FILE = os.path.join(self.db.PATH, 'itypes.yaml')

            contents = load_card_node(self.DATA_FILE, schemefile=self.SCHEME_FILE, cacher=self.db.cacher)

            for value in contents:
                itype = IType(self, value)
                if itype.name in self.itypes:
                    raise Exception('Itype <%s> mentioned twice in <%s>' % (itype.name, self.DATA_FILE))
                self.itypes[itype.name] = itype

        self.modified = False

    def has_itype(self, name):
        return name in self.itypes

    def get_itype(self, name):
        return self.itypes[name]

    def get_itypes(self):
        return self.itypes.values()

    def get_groups_with_itype(self, name):
        return filter(lambda x: x.card.tags.itype == name, self.db.groups.get_groups())

    def add_itype(self, itype):
        """
            Function adds already constructed itype

            :type itype: core.itypes.Itype

            :param itype: new itype
        """

        self.itypes[itype.name] = itype

        self.mark_as_modified()

    def remove_itype(self, itype):
        groups_with_itype = filter(lambda x: x.card.tags.itype == itype, self.db.groups.get_groups())
        if len(groups_with_itype) > 0:
            raise Exception("Can not remove ctype <%s>, which is used by <%s>" % (
                itype, ",".join(map(lambda x: x.card.name, groups_with_itype))))

        self.itypes.pop(itype)

        self.mark_as_modified()

    def rename_itype(self, old_name, new_name):
        if self.has_itype(new_name):
            raise Exception("Renaming ctype <%s> to already existing ctype <%s>" % (old_name, new_name))

        for group in filter(lambda x: x.card.tags.itype == old_name, self.db.groups.get_groups()):
            group.card.tags.itype = new_name
            group.mark_as_modified()

        self.itypes[old_name].name = new_name
        self.itypes[new_name] = self.itypes[old_name]
        self.itypes.pop(old_name)

        self.mark_as_modified()

    def update(self, smart=False):
        if self.db.version <= "2.2.16":
            return
        if smart and not self.modified:
            return
        save_card_node(sorted(self.itypes.values(), cmp=lambda x, y: cmp(x.name, y.name)), self.DATA_FILE,
                       self.SCHEME_FILE)

    def fast_check(self, timeout):
        # nothing to check
        pass

    def mark_as_modified(self):
        self.modified = True
