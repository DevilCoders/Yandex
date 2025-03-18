"""
    Cpu models classes and interface
"""

import os
from collections import OrderedDict

import re
import ujson
import json
from _ctypes import PyObj_FromPtr

from core.card.node import CardNode, load_card_node, save_card_node


class NoIndent(object):
    """ Value wrapper. """
    def __init__(self, value):
        self.value = value


class MyEncoder(json.JSONEncoder):
    FORMAT_SPEC = '@@{}@@'
    regex = re.compile(FORMAT_SPEC.format(r'(\d+)'))

    def __init__(self, **kwargs):
        # Save copy of any keyword argument values needed for use here.
        self.__sort_keys = kwargs.get('sort_keys', None)
        super(MyEncoder, self).__init__(**kwargs)

    def default(self, obj):
        return (self.FORMAT_SPEC.format(id(obj)) if isinstance(obj, NoIndent)
                else super(MyEncoder, self).default(obj))

    def encode(self, obj):
        format_spec = self.FORMAT_SPEC  # Local var to expedite access.
        json_repr = super(MyEncoder, self).encode(obj)  # Default JSON.

        # Replace any marked-up object ids in the JSON repr with the
        # value returned from the json.dumps() of the corresponding
        # wrapped Python object.
        for match in self.regex.finditer(json_repr):
            # see https://stackoverflow.com/a/15012814/355230
            id = int(match.group(1))
            no_indent = PyObj_FromPtr(id)
            json_obj_repr = json.dumps(no_indent.value, sort_keys=self.__sort_keys)

            # Replace the matched id string with json formatted representation
            # of the corresponding Python object.
            json_repr = json_repr.replace(
                            '"{}"'.format(format_spec.format(id)), json_obj_repr)

        return json_repr


class TCpuModelsInfo(object):
    __slots__ = ['db', 'datafile', 'schemefile', 'models', 'fullname_to_model', 'botmodel_to_model', 'modified']


    def __init__(self, db):
        self.db = db

        if self.db.version <= '0.7':
            self.datafile = os.path.join(self.db.PATH, 'hardware_data', 'models_power')
        else:
            self.datafile = os.path.join(self.db.HDATA_DIR, 'models.yaml')
        self.schemefile = os.path.join(self.db.SCHEMES_DIR, 'cpumodels.yaml')

        self.models = OrderedDict()
        self.fullname_to_model = {}
        self.botmodel_to_model = {}

        self.modified = False

        self.load()

    def load(self):
        if self.db.version <= '0.7':
            cpu_models = []
            for line in open(self.datafile).readlines():
                name, power, ncpu = line.strip().split(' ')
                power = float(power)
                ncpu = int(ncpu)
                d = {'name': name, 'fullname': name, 'power': power, 'ncpu': ncpu, 'freq': 1,
                     'tbfreq': 1, 'qpsbyload': [power] * ncpu }
                cpu_models.append(type("DummyCpuModel", (), d)())
            cpu_models = OrderedDict(map(lambda x: (x.name, x), cpu_models))
            if self.db.version <= '0.5':
                cpu_models['unknown'] = type("DummyCpuModel", (), {'name': 'unknown', 'power': 1, 'ncpu': 1})()
        elif self.db.version <= '0.9.5':
            cpu_models = OrderedDict()
            for elem in yaml.load(open(self.datafile).read()):
                cpu_models[elem['model']] = type("DummyCpuModel", (), elem)()
        else:
#            if self.db.version >= '2.2.52':
#                cpu_models = ujson.loads(open(self.datafile).read())
#                cpu_models = [CardNode.create_from_dict(x) for x in cpu_models]
#                cpu_models = OrderedDict(map(lambda x: (x['model'], x), cpu_models))
#            else:
#                cpu_models = load_card_node(self.datafile, self.schemefile, cacher=self.db.cacher)
#                cpu_models = OrderedDict(map(lambda x: (x['model'], x), cpu_models))
            cpu_models = load_card_node(self.datafile, self.schemefile, cacher=self.db.cacher)
            cpu_models = OrderedDict(map(lambda x: (x['model'], x), cpu_models))
            fullname_to_model = {v['fullname']: k for k, v in cpu_models.items()}
            botmodel_to_model = {botmodel.lower(): k for k, v in cpu_models.items() for botmodel in v['botmodel']}

        self.models.update(cpu_models)
        self.fullname_to_model.update(fullname_to_model)
        self.botmodel_to_model.update(botmodel_to_model)

    def get_model_names(self):
        return self.models.keys()

    def get_model(self, model_name):
        return self.models[model_name]

    def get_model_by_fullname(self, fullname, default=None):
        return self.fullname_to_model.get(fullname, default)

    def get_model_by_botmodel(self, botmodel, default="unknown"):
        return self.botmodel_to_model.get(botmodel.lower(), default)

    def update(self, smart=False):
        # FIXME: smart with partial update is not supported yet
        if (not smart) or self.modified:
#            if self.db.version >= '2.2.52':
#                result = []
#                for model in self.models.itervalues():
#                    model_dict = model.as_dict()
#
#                    # prettify output
#                    model_dict['powerbyload']['tbon'] = NoIndent(model_dict['powerbyload']['tbon'])
#                    model_dict['powerbyload']['tboff'] = NoIndent(model_dict['powerbyload']['tboff'])
#                    model_dict['tbfreq'] = NoIndent(model_dict['tbfreq'])
#                    for elem in model_dict['consumption']:
#                        for signal in ('qps', 'cons', 'corecons', 'pkgcons', 'ramcons', 'cputemp', 'eff_freq'):
#                            elem[signal]['tbon'] = NoIndent(elem[signal]['tbon'])
#                            elem[signal]['tboff'] = NoIndent(elem[signal]['tboff'])
#
#                    result.append(model_dict)
#                with open(self.datafile, 'w') as f:
#                    f.write(json.dumps(result, cls=MyEncoder, sort_keys=True, indent=2))
#            else:
#                save_card_node(self.models.values(), self.datafile, self.schemefile)
            save_card_node(self.models.values(), self.datafile, self.schemefile)

    def fast_check(self, timeout):
        del timeout

    def mark_as_modified(self):
        self.modified = True
