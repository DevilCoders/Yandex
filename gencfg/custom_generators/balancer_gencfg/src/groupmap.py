# -*- coding: utf-8 -*

import os
import re
import logging
import cPickle

from src.mapping import versions


class TGroupToTagMapping(object):
    """
    Returns tag for group/intlookup
    """

    TRUNK = "trunk"

    class ETypes:
        GROUP = "group"
        INTLOOKUP = "intlookup"
        ALL = [GROUP, INTLOOKUP]

    def __init__(self, trunk_fallback=True):
        jsoned = versions
        self.trunk_fallback = trunk_fallback

        self.mapping = dict()
        # accessed group names
        self.groups_cache = set()

        for elem in jsoned:
            etype = elem['type']
            name = elem['name']
            tag = elem['tag']

            if (etype, name) in self.mapping:
                raise Exception("Entity <{}, {}> specified twice in config".format(etype, name))

            if etype not in TGroupToTagMapping.ETypes.ALL:
                raise Exception("Unknown entity type <{}>".format(etype))

            if tag != TGroupToTagMapping.TRUNK and not re.match(r'stable-\d+-r\d+', tag):
                raise Exception("Tagname <{}> is not a tag name".format(tag))

            self.mapping[(etype, name)] = tag

    def gettag(self, etype, name):
        self.groups_cache.add(name)

        if (etype, name) not in self.mapping:
            logging.info("GenCFG group %s %s not found, falling back to trunk", etype, name)
            if self.trunk_fallback:
                # SEPE-14629
                return TGroupToTagMapping.TRUNK

            raise Exception(
                "Entity <{}, {}> not found in tag mapping. This is forbidden, see SEARCH-7119. ".format(
                    etype, name,
                ))

        return self.mapping[(etype, name)]

    def load_groups_cache(self, file_name):
        if os.path.exists(file_name):
            with open(file_name) as f:
                self.groups_cache = cPickle.load(f)

    def save_groups_cache(self, file_name):
        with open(file_name, 'wb') as f:
            # use highest pickle protocol
            cPickle.dump(self.groups_cache, f, -1)
