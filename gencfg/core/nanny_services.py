"""Local cache of nanny services for faster exporting to cauth (RX-354)"""

import os

from core.card.node import CardNode, Scheme, load_card_node, save_card_node


class NannyService(CardNode):
    """Nanny service as gencfg entity"""

    def __init__(self, parent, name, owners, hosts):
        super(NannyService, self).__init__()

        self.parent = parent
        self.name = name
        self.owners = owners
        self.hosts = hosts


class NannyServices(object):
    def __init__(self, db):
        self.db = db

        self.data = dict()

        if self.db.version >= '2.2.43':
            self.SCHEME_FILE = os.path.join(self.db.SCHEMES_DIR, 'nanny_services.yaml')
            self.DATA_FILE = os.path.join(self.db.PATH, 'nanny_services.yaml')

            contents = load_card_node(self.DATA_FILE, schemefile=self.SCHEME_FILE, cacher=self.db.cacher)

            for value in contents:
                nanny_service = NannyService(self, value.name, value.owners, value.hosts)
                if nanny_service.name in self.data:
                    raise Exception('Nanny service <%s> mentioned twice in <%s>' % (nanny_service.name, self.DATA_FILE))
                self.data[nanny_service.name] = nanny_service

        self.modified = False


    def has(self, name):
        return name in self.data

    def get(self, name):
        return self.data[name]

    def get_all(self):
        return self.data.values()

    def add(self, service_name, owners, hosts):
        if service_name in self.data:
            raise Exception('Nanny service <{}> already in database'.format(service_name))

        nanny_service = NannyService(self, service_name, owners, hosts)
        self.data[service_name] = nanny_service
        self.mark_as_modified()

    def remove(self, service):
        if isinstance(service, str):
            service = self.get(service)

        self.data.pop(service.name)
        self.mark_as_modified()

    def update(self, smart=False):
        if self.db.version < '2.2.43':
            return
        if smart and not self.modified:
            return

        save_card_node(sorted(self.data.values(), cmp=lambda x, y: cmp(x.name, y.name)), self.DATA_FILE,
                       self.SCHEME_FILE)

    def fast_check(self, timeout):
        # nothing to check
        pass

    def mark_as_modified(self):
        self.modified = True

