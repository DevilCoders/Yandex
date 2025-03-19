import json


class Base(object):
    def __str__(self):
        return json.dumps(self.as_dict())

    def as_dict(self):
        return self.__dict__


class MRTable(Base):
    def __init__(self, cluster, table):
        self.cluster = cluster
        self.table = table


class MRDirectory(Base):
    def __init__(self, cluster, path):
        self.cluster = cluster
        self.path = path
