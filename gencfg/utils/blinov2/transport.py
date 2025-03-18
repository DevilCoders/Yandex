from collections import defaultdict
import xmlrpclib
import os

import gencfg
from core.hosts import FakeHost
from core.instances import FakeInstance
import core.argparse.types as argparse_types


class EObjectTypes(object):
    HOST = "HOST"
    HGROUP = "HGROUP"
    STAG = "STAG"
    SHARD = "SHARD"
    ITAG = "ITAG"
    CONF = "CONF"
    DC = "DC"
    LINE = "LINE"


class ITransport(object):
    """
        Interface for transport to get instances(hosts?) from gencfg/skyet/???
    """

    def __init__(self):
        pass

    """
        Iterator return SORTED sequence of instances/hosts
    """

    def iterate(self, object_type, object_name):
        raise NotImplementedError("Function <iterate> not implemented")


class ISearcherlookupTransport(ITransport):
    TYPE = "sl"

    def __init__(self, path):
        super(ISearcherlookupTransport, self).__init__()
        if path == 'default':
            path = os.path.join(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')), 'w-generated',
                                'searcherlookup.conf')

        self.itags = {}

        self.lines = map(lambda x: x.strip(), open(os.path.expanduser(path)).readlines())
        for (linenum, line) in enumerate(self.lines):
            if line.startswith('%instance_tag ') or line.startswith('%instance_tag_auto '):
                self.itags[line.partition(' ')[2]] = linenum

    def iterate(self, object_type, object_name):
        if object_type == EObjectTypes.HOST:
            yield FakeHost(object_name)
        elif object_type == EObjectTypes.HGROUP:
            prev_host = None
            for item in self.iterate_itag(object_name):
                if prev_host is None or prev_host != item.host:
                    yield item.host
                prev_host = item.host
        elif object_type == EObjectTypes.ITAG:
            for item in self.iterate_itag(object_name):
                yield item
        elif object_type == EObjectTypes.DC:
            for item in self.iterate_itag('a_dc_%s' % object_name):
                yield item
        elif object_type == EObjectTypes.LINE:
            for item in self.iterate_itag('a_line_%s' % object_name):
                yield item
        else:
            raise Exception("Unsupported object type %s for <searcherlookup> transport" % object_type)


class TSearcherlookupGencfgTransport(ISearcherlookupTransport):
    TYPE = "sl"

    def __init__(self, path):
        super(TSearcherlookupGencfgTransport, self).__init__(path)
        if path == 'default':
            from core.db import CURDB
            db = CURDB
        else:
            from core.db import DB
            db = DB(os.path.expanduser(path))

        self.db_slookup = db.build_searcherlookup()

    def iterate_itag(self, itag):
        if itag not in self.db_slookup.itags_auto:
            raise Exception("No itag <%s> in searcherlookup" % itag)

        instances = self.db_slookup.itags_auto.get(itag, [])
        instances = list(set(instances))
        instances.sort(cmp=lambda x, y: cmp(x.name(), y.name()))

        if len(instances) == 0:
            raise Exception("No itag <%s> in gencfg searcherlookup" % itag)

        for instance in instances:
            yield FakeInstance(FakeHost(instance.host.name.partition('.')[0]), instance.port)


class TSearcherlookupFileTransport(ISearcherlookupTransport):
    TYPE = "slf"

    def __init__(self, path):
        super(TSearcherlookupFileTransport, self).__init__(path)
        if path == 'default':
            path = os.path.join(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')), 'w-generated',
                                'searcherlookup.conf')

        self.itags = {}

        self.lines = map(lambda x: x.strip(), argparse_types.some_content(path).split('\n'))
        for (linenum, line) in enumerate(self.lines):
            if line.startswith('%instance_tag ') or line.startswith('%instance_tag_auto '):
                self.itags[line.partition(' ')[2]] = linenum

    def valid_object(self, line):
        if line.startswith('%') or line.strip() == '':
            return False
        return True

    def iterate_itag(self, itag):
        if itag not in self.itags:
            raise Exception("No itag <%s> in searcherlookup" % itag)
        for i in range(self.itags[itag] + 1, len(self.lines)):
            if self.valid_object(self.lines[i]):
                host, _, port = self.lines[i].partition(':')
                port = int(port)
                yield FakeInstance(FakeHost(host), port)
            else:
                break


class TCmsTransport(ITransport):
    TYPE = "cms"

    URL = "http://cmsearch.yandex.ru/xmlrpc/bs"

    def __init__(self, configuration_name):
        super(TCmsTransport, self).__init__()
        if configuration_name == 'default':
            self.conf = 'HEAD'
        else:
            self.conf = configuration_name

        self.proxy = xmlrpclib.ServerProxy(self.URL)

    def iterate_with_filter(self, flt):
        bulk_data = self.proxy.listSearchInstances_bulk(flt)
        for line in bulk_data.split('\n'):
            line.strip()
            for instance in line.split(' ')[1:]:
                host, _, port = instance.partition(':')
                port = int(port)
                yield FakeInstance(FakeHost(host), port)

    def iterate(self, object_type, object_name):
        if object_type == EObjectTypes.HOST:
            yield FakeHost(object_name)
        elif object_type in [EObjectTypes.ITAG, EObjectTypes.STAG, EObjectTypes.SHARD]:
            if object_type == EObjectTypes.ITAG:
                d = {'conf': self.conf, 'instanceTagName': object_name}
            elif object_type == EObjectTypes.STAG:
                d = {'conf': self.conf, 'shardTagName': object_name}
            elif object_type == EObjectTypes.SHARD:
                d = {'conf': self.conf, 'shard': object_name}
            for instance in self.iterate_with_filter(d):
                yield instance
        else:
            raise Exception("Unsupported object type %s for <cms> transport" % object_type)


class TGencfgGroupsTransport(ITransport):
    TYPE = "gg"

    def __init__(self, path, iterate_instances=False):
        super(TGencfgGroupsTransport, self).__init__()
        if path == 'default':
            from core.db import CURDB
            self.db = CURDB
        else:
            from core.db import DB
            self.db = DB(os.path.expanduser(path))

        self.iterate_instances = iterate_instances

    def iterate(self, object_type, object_name):
        if object_type == EObjectTypes.ITAG:
            if object_name.startswith('a_itype'):
                flt = lambda x: x.card.tags.itype == object_name[8:]
            elif object_name.startswith('a_ctype'):
                flt = lambda x: x.card.tags.ctype == object_name[8:]
            elif object_name.startswith('a_prj'):
                flt = lambda x: object_name[6:] in x.card.tags.prj
            elif object_name.startswith('a_metaprj'):
                flt = lambda x: x.card.tags.metaprj == object_name[10:]
            elif object_name.startswith('itag'):
                flt = lambda x: object_name[5:] in x.card.tags.itag
            elif object_name.isupper():
                flt = lambda x: x.card.name == object_name
            else:
                raise Exception("Unknown filter object name <%s>" % object_name)
        else:
            raise Exception("Unsupported object_type %s for GencfgGroupTransport" % object_type)

        for group in filter(flt, self.db.groups.get_groups()):
            if self.iterate_instances:
                for instance in group.get_kinda_busy_instances():
                    yield instance
            else:
                yield group.card.name


class TMultiTransport(object):
    def __init__(self, default):
        if isinstance(default, ITransport):
            self.transport_cache = {'default': default}
        else:
            self.transport_cache = {'default': self.create_transport(default)}

    def create_transport(self, transport_descr):
        if transport_descr.find(':') > 0:
            transport_type, _, transport_param = transport_descr.partition(':')
        else:
            transport_type, transport_param = transport_descr, 'default'

        if transport_type == 'sl':
            return TSearcherlookupGencfgTransport(transport_param)
        elif transport_type == 'slf':
            return TSearcherlookupFileTransport(transport_param)
        elif transport_type == 'cms':
            return TCmsTransport(transport_param)
        elif transport_type == 'gg':
            return TGencfgGroupsTransport(transport_param)
        else:
            raise Exception("Unknown transport type %s" % transport_type)

    def iterate(self, object_type, object_name, object_transport):
        if object_transport not in self.transport_cache:
            self.transport_cache[object_transport] = self.create_transport(object_transport)

        for elem in self.transport_cache[object_transport].iterate(object_type, object_name):
            yield elem
