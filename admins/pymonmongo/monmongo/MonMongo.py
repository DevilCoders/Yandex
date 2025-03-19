import pymongo
import bson
import json
import re
import time
import datetime
from .ProfileTailer import ProfileTailer
from .Counter import Counter
from .QueryHasher import QueryHasher


class MonMongo:

    def __init__(self):
        self.counter = Counter()

    def process_command(self, data):
        """ Process the 'command' operation """
        command = QueryHasher.normalize(data['command'])
        self.counter.inc('', 'command', command, data['millis'])

    def process_update(self, data):
        """ Process the 'update' operation """
        collname = data['ns']
        query = QueryHasher.normalize(data['query'])
        update = QueryHasher.normalize(data['updateobj'])
        self.counter.inc(collname, 'update', query + ' -> ' + update, data['millis'])

    def process_insert(self, data):
        """ Process the 'insert' operation """
        collname = data['ns']
        query = QueryHasher.normalize(data['query'])
        self.counter.inc(collname, 'insert', query, data['millis'])

    def process_remove(self, data):
        """ Process the 'remove' operation """
        collname = data['ns']
        query = QueryHasher.normalize(data['query'])
        self.counter.inc(collname, 'remove', query, data['millis'])

    def process_query(self, data):
        """ Process the 'query' operation """
        collname = data['ns']
        opdata = data['query']
        if isinstance(opdata, str):
            opdata = {'filter': "[unparsable string]"}
        query = QueryHasher.normalize(opdata['filter']) if 'filter' in opdata else ""
        self.counter.inc(collname, 'query', query, data['millis'])

    def process_getmore(self, data):
        """ Process the 'getmore' operation """
        collname = data['ns']
        opdata = data['originatingCommand']
        if isinstance(opdata, str):
            query = "[unloadable sequence due to a mongodb bug]"
        else:
            query = QueryHasher.normalize(opdata['filter']) if 'filter' in opdata else ""
        self.counter.inc(collname, 'getmore', query, data['millis'])

    def process_killcursors(self, data):
        """ Process the 'killcursors' operation """
        self.counter.inc('', 'killcursors', '', data['millis'])

    def process(self, row):
        """ Process a generic row """
        op = row['op']
        handler = getattr(self, 'process_' + op)
        try:
            handler(row)
        except BaseException as x:
            print(row)
            raise

    def run(self, url, collections, queries, period):
        client = pymongo.MongoClient(url)

        admin_db = client['admin']
        dblist = [x['name'] for x in admin_db.command({'listDatabases': 1})['databases']]

        tailers = [ProfileTailer(client, x, self.process) for x in dblist]
        for tailer in tailers:
            tailer.start()

        while True:
            try:
                time.sleep(period)
            except KeyboardInterrupt:
                break
            counter = self.counter
            self.counter = Counter()
            counter.dump(collections, queries)

        for tailer in tailers:
            tailer.stop()
        for tailer in tailers:
            tailer.join()
