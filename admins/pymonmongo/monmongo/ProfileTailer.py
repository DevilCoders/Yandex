import pymongo
import pymongo.database
import threading


class ProfileTailer(threading.Thread):
    def __init__(self, client: pymongo.MongoClient, dbname: str, handler):
        super().__init__()
        self.dbname = dbname
        self.collection = client[dbname]['system.profile']
        self.handler = handler

        self.running = True

    def stop(self):
        self.running = False

    def run(self):
        try:
            first_row = self.collection.find().sort('$natural', pymongo.DESCENDING).limit(1).next()
        except StopIteration:
            print("Got StopIteration for db {} (profiling disabled?)".format(self.dbname))
            return

        for tries in range(3):
            cursor = self.collection.find({
                                              'ts': {
                                                  '$gt': first_row['ts']
                                              },
                                              'millis': {
                                                  '$gte': 0
                                              },
                                              'ns': {
                                                  '$ne': '{}.system.profile'.format(self.dbname)
                                              },
                                          },
                                          cursor_type=pymongo.CursorType.TAILABLE_AWAIT).sort('$natural')
            for row in cursor:
                self.handler(row)
                if not self.running:
                    return
