"""
    Disk models classes and interface
"""

import os
from collections import OrderedDict

from core.card.node import load_card_node, save_card_node

class TDiskModelsInfo(object):
    __slots__ = ['db', 'datafile', 'schemefile', 'models', 'modified']

    def __init__(self, db):
        self.db = db

        self.datafile = os.path.join(self.db.HDATA_DIR, 'diskmodels.yaml')
        self.schemefile = os.path.join(self.db.SCHEMES_DIR, 'diskmodels.yaml')

        self.models = OrderedDict()

        self.modified = False

        self.load()

    def load(self):
        if self.db.version < '2.2.31': # no data in older bases
            disk_models = OrderedDict()
        else:
            disk_models = load_card_node(self.datafile, self.schemefile, cacher=self.db.cacher)
            disk_models = OrderedDict(map(lambda x: (x['model'], x), disk_models))

        self.models.update(disk_models)

    def get_model_names(self):
        return self.models.keys()

    def get_model(self, model_name):
        return self.models[model_name]

    def get_models(self):
        return self.models.values()

    def update(self, smart=False):
        # FIXME: smart with partial update is not supported yet
        if (not smart) or self.modified:
            save_card_node(self.models.values(), self.datafile, self.schemefile)

    def fast_check(self, timeout):
        del timeout

    def mark_as_modified(self):
        self.modified = True
