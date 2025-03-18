"""
    Instance types classes and interface.
"""

import os

from core.card.node import CardNode, Scheme, load_card_node, save_card_node


class CType(CardNode):
    def __init__(self, parent, card_content):
        """
            CType node constructor.

            :param parent(core.ctypes.CTypes): container of ctypes
            :param card_content(CardNode or dict): card node loaded from yaml-file (dict for older versions)
        """

        super(CType, self).__init__()
        self.parent = parent

        for child in card_content:
            self.__dict__[child] = card_content[child]


class CTypes(object):
    def __init__(self, db):
        self.db = db

        self.ctypes = dict()

        if self.db.version <= "2.2.7":  # initialize by getting all ctypes from groups
            for ctype_name in set(map(lambda x: x.card.tags.ctype, self.db.groups.get_groups())):
                self.ctypes[ctype_name] = CType(self, {'name': ctype_name})
        elif self.db.version <= "2.2.16":  # initialize from content of dbconstans
            for ctype_name in self.db.constants.CTYPES:
                d = {'name': ctype_name, 'descr': "No description"}
                self.ctypes[ctype_name] = CType(self, d)
        else:  # initialize from ctypes.yaml
            self.SCHEME_FILE = os.path.join(self.db.SCHEMES_DIR, 'ctypes.yaml')
            self.DATA_FILE = os.path.join(self.db.PATH, 'ctypes.yaml')

            contents = load_card_node(self.DATA_FILE, schemefile=self.SCHEME_FILE, cacher=self.db.cacher)

            for value in contents:
                ctype = CType(self, value)
                if ctype.name in self.ctypes:
                    raise Exception('CType <%s> mentioned twice in <%s>' % (ctype.name, self.DATA_FILE))
                self.ctypes[ctype.name] = ctype

        self.modified = False

    def has_ctype(self, name):
        return name in self.ctypes

    def get_ctype(self, name):
        return self.ctypes[name]

    def get_ctypes(self):
        return self.ctypes.values()

    def get_groups_with_ctype(self, name):
        return filter(lambda x: x.card.tags.ctype == name, self.db.groups.get_groups())

    def add_ctype(self, ctype):
        """
            Function adds already constructed ctype

            :type ctype: core.ctypes.CType

            :param ctype: new ctype
        """

        self.ctypes[ctype.name] = ctype

        self.mark_as_modified()

    def remove_ctype(self, ctype):
        groups_with_ctype = filter(lambda x: x.card.tags.ctype == ctype, self.db.groups.get_groups())
        if len(groups_with_ctype) > 0:
            raise Exception("Can not remove ctype <%s>, which is used by <%s>" % (
                ctype, ",".join(map(lambda x: x.card.name, groups_with_ctype))))

        self.ctypes.pop(ctype)

        self.mark_as_modified()

    def rename_ctype(self, old_name, new_name):
        if self.has_ctype(new_name):
            raise Exception("Renaming ctype <%s> to already existing ctype <%s>" % (old_name, new_name))

        for group in filter(lambda x: x.card.tags.ctype == old_name, self.db.groups.get_groups()):
            group.card.tags.ctype = new_name
            group.mark_as_modified()

        self.ctypes[old_name].name = new_name
        self.ctypes[new_name] = self.ctypes[old_name]
        self.ctypes.pop(old_name)

        self.mark_as_modified()

    def update(self, smart=False):
        if self.db.version <= "2.2.16":
            return
        if smart and not self.modified:
            return

        save_card_node(sorted(self.ctypes.values(), cmp=lambda x, y: cmp(x.name, y.name)), self.DATA_FILE,
                       self.SCHEME_FILE)

    def fast_check(self, timeout):
        # nothing to check
        pass

    def mark_as_modified(self):
        self.modified = True
