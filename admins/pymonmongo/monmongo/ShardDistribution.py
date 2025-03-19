import pymongo
import pymongo.errors
from .Counter import red, green, blue
from termcolor import colored


def gray(text):
    return colored(text, 'grey', attrs=['bold'])


class ShardDistribution:

    @staticmethod
    def format_size(size: float):
        if size > 1073741824:
            return '{:6.1f}G'.format(size / 1073741824)
        if size > 1048576:
            return '{:6.1f}M'.format(size / 1048576)
        if size > 1024:
            return '{:6.1f}K'.format(size / 1024)
        return '{:6.1f}'.format(size)

    def run(self, url):
        client = pymongo.MongoClient(url)

        admin_db = client['admin']
        dblist = [x['name'] for x in admin_db.command({'listDatabases': 1})['databases']]

        for dbname in sorted(dblist):
            db = client[dbname]
            collection_names = [x['name'] for x in db.command("listCollections")['cursor']['firstBatch']]

            print(blue(dbname))
            for collection in sorted(collection_names):
                try:
                    stats = db.command({'collStats': collection})
                except pymongo.errors.OperationFailure:
                    continue
                sharded = stats['sharded']

                if sharded:
                    shards = 0
                    size_str = ''
                    for shard_name, shard in stats['shards'].items():
                        shards += 1
                        size_str += green(self.format_size(shard['size'])) + ' '
                else:
                    shards = 1
                    size = stats['size']
                    if size < 104857600:        # 100M
                        size_str = gray(self.format_size(size))
                    else:
                        size_str = red(self.format_size(size))

                print('    {:40s} {} {}'.format(collection, shards, size_str))
