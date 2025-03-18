"""Hbf macroses classes and interface"""

import os
import copy

import json
import ujson

from collections import defaultdict

from gaux.aux_mongo import get_next_hbf_project_id
from gaux.aux_staff import unwrap_dpts
from core.card.node import CardNode, Scheme, load_card_node, save_card_node


class HbfMacros(CardNode):
    def __init__(self, parent, name, parent_macros=None, description=None, owners=None, hbf_project_id=None, removed=False, external=False, skip_owners_check=False, group_macro=False):
        """
            HbfMacro node constructor.

            :param parent(core.hbfmacroses.HbfMacroses): container of hbf macroses
            :param parent_macros(core.hbfmacroses.HbfMacros): parent macros
        """

        if (description is None) or (owners is None):
            raise Exception('Missing obligatory param')

        super(HbfMacros, self).__init__()
        self.parent = parent

        self.name = name
        self.parent_macros = parent_macros
        self.description = description
        self.owners = copy.copy(owners)
        self.hbf_project_id = hbf_project_id
        self.removed = removed
        self.external = external
        self.skip_owners_check = skip_owners_check
        self.group_macro = group_macro

    @property
    def child_macroses(self):
        return self._child_macroses()

    def _child_macroses(self):
        return [x for x in self.parent.data.itervalues() if (x.parent_macros == self.name) and (x.removed == False)]

    @property
    def recurse_child_macroses(self):
        return self._recurse_child_macroses()

    def _recurse_child_macroses(self):
        child_macroses = self.child_macroses
        for child_macros in child_macroses:
            child_macroses.extend(child_macros._recurse_child_macroses())
        return child_macroses

    @property
    def resolved_owners(self):
        """Resolve all staff/abc groups and return owners"""
        return self._resolved_owners()

    def _resolved_owners(self):
        return unwrap_dpts(self.owners, suppress_missing=True)

    def as_dict(self):
        return dict(name=self.name, parent_macros=self.parent_macros, description=self.description, hbf_project_id=self.hbf_project_id,
                    owners=self.owners, removed=self.removed, external=self.external, skip_owners_check=self.skip_owners_check, group_macro=self.group_macro)

    def to_json(self):
        return dict(name=self.name, parent_macros=self.parent_macros, description=self.description, hbf_project_id=self.hbf_project_id,
                    owners=self.owners, removed=self.removed, external=self.external, resolved_owners=self.resolved_owners, skip_owners_check=self.skip_owners_check,
                    group_macro=self.group_macro)


class HbfMacroses(object):
    def __init__(self, db):
        self.db = db

        self.SCHEME_FILE = os.path.join(self.db.SCHEMES_DIR, 'hbfmacroses.yaml')
        self.DATA_FILE = os.path.join(self.db.PATH, 'hbfmacroses.yaml')

        self.data = dict()
        if self.db.version <= "2.2.37":  # initialize by getting all macroses from groups
            hbf_macro_owners = defaultdict(set)
            for group in self.db.groups.get_groups():
                hbf_macro_owners[getattr(group.card.properties, 'hbf_parent_macros', 'NO_MACROS')] |= set(group.card.owners)
            hbf_macro_owners['_GENCFG_SEARCHPRODNETS_ROOT_'] = set()

            for macro_name, macro_owners in hbf_macro_owners.iteritems():
                self.data[macro_name] = HbfMacros(self, macro_name, description='No description', owners=macro_owners, hbf_project_id=0)
        else:  # initialize from hbfmacroses.yaml
            if self.db.version >= "2.2.51":
                contents = ujson.loads(open(self.DATA_FILE).read())
                contents = [CardNode.create_from_dict(x) for x in contents]
            else:
                contents = load_card_node(self.DATA_FILE, schemefile=self.SCHEME_FILE, cacher=self.db.cacher)

            for value in contents:
                hbf_macros = HbfMacros(self, value.name, parent_macros=value.parent_macros, description=value.description, owners=value.owners,
                                       hbf_project_id=value.hbf_project_id, removed=getattr(value, 'removed', False),
                                       external=getattr(value, 'external', False), skip_owners_check=getattr(value, 'skip_owners_check', False),
                                       group_macro=getattr(value, 'group_macro', False))
                if hbf_macros.name in self.data:
                    raise Exception('Macro <%s> mentioned twice in <%s>' % (hbf_macros.name, self.DATA_FILE))
                self.data[hbf_macros.name] = hbf_macros

        self.modified = False

    def has_hbf_macros(self, name):
        return name in self.data

    def get_hbf_macros(self, name):
        return self.data[name]

    def get_hbf_macroses(self):
        return self.data.values()

    def add_hbf_macros(self, name, description, owners, parent_macros=None, hbf_project_id=None):
        """Function adds already constructed hbf macros

            :type hbf_macros: core.hbfmacroses.HbfMacros

            :param hbf_macros: new hbf macros
        """

        if parent_macros is not None:
            if isinstance(parent_macros, str):
                parent_macros_name = parent_macros
            else:
                parent_macros_name = parent_macros.name
        else:
            parent_macros_name = None

        if hbf_project_id is None:
            print('GENERATED PROJECT ID FOR MACRO')
            hbf_project_id = get_next_hbf_project_id()
        elif hbf_project_id == -1:  # Migration for disable generate project_id for macro
            hbf_project_id = None
        hbf_macros = HbfMacros(self, name, parent_macros=parent_macros_name, description=description, owners=owners, hbf_project_id=hbf_project_id)

        self.data[hbf_macros.name] = hbf_macros

        self.mark_as_modified()

        return hbf_macros

    def remove_hbf_macros(self, hbf_macros):
        groups_with_hbf_macros = filter(lambda x: x.card.properties.hbf_parent_macros == hbf_macros, self.db.groups.get_groups())
        if len(groups_with_hbf_macros) > 0:
            raise Exception("Can not remove hbf macros <%s>, which is used by <%s>" % (
                hbf_macros, ",".join(map(lambda x: x.card.name, groups_with_hbf_macros))))

        self.data.pop(hbf_macros)

        self.mark_as_modified()

    def get_groups_with_hbf_macros(self, name):
        return filter(lambda x: x.card.properties.hbf_parent_macros == name, self.db.groups.get_groups())

    def update(self, smart=False):
        if self.db.version <= "2.2.37":
            return
        if smart and not self.modified:
            return
        if self.db.version >= "2.2.51":
            result = [x.as_dict() for x in sorted(self.data.values(), key=lambda x: x.name)]
            with open(self.DATA_FILE, 'w') as f:
                f.write(json.dumps(result, indent=4, sort_keys=True))
        else:
            save_card_node(sorted(self.data.values(), cmp=lambda x, y: cmp(x.name, y.name)), self.DATA_FILE,
                           self.SCHEME_FILE)

    def fast_check(self, timeout):
        # nothing to check
        pass

    def mark_as_modified(self):
        self.modified = True
