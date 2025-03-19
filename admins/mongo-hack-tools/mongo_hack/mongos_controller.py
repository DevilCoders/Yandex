from base_controller import BaseController
from misc import log
from time import sleep
import re


class MongosController (BaseController):

    def __init__(self, host):
        super(MongosController, self).__init__(host, 27017, '/etc/mongos.conf', 'mongos')
        self.config = self.parse_config("CFGDB=" + self.ssh.run(
            'mongo --quiet  --eval "print(db.serverCmdLineOpts().parsed.sharding.configDB)"', True))

    @log
    def disable_balancer(self):
        db = self.mongo.config
        db.settings.update({ '_id': 'balancer' }, { '$set': { 'stopped': True } })
        while True:
            x = db.locks.find_one({'_id': 'balancer'})
            if x and x['state'] > 0:
                sleep(1)
            else:
                break

    @log
    def enable_balancer(self):
        self.mongo.config.settings.update({ '_id': 'balancer' }, { '$set': { 'stopped': False } })

    @log
    def rename_rs(self, from_name, to_name, new_host_list):
        db = self.mongo.config
        db.databases.update({'primary': from_name}, { '$set': {'primary': to_name} }, multi=True)
        db.chunks.update({'shard': from_name}, { '$set': {'shard': to_name} }, multi=True)

        data = db.shards.find_one({'_id': from_name})
        data['_id'] = to_name
        data['host'] = to_name + '/' + ','.join(["%s:27018" % x for x in new_host_list])
        db.shards.remove({'_id': from_name})
        db.shards.insert(data)

    def host_list(self, shard_name):
        db = self.mongo.config
        hostconf = None
        for shard in db.shards.find():
            if shard['_id'] == shard_name:
                hostconf = shard['host']
                break
        else:
            raise RuntimeError('Shard %s name not found' % shard_name)

        return [x.split(':')[0] for x in hostconf.split('/')[1].split(',')]

    def get_shards(self):
        db = self.mongo.config
        result = []
        for shard in db.shards.find():
            result.append((shard['_id'], [x.split(':')[0] for x in shard['host'].split('/')[1].split(',')]))
        return sorted(result, key=lambda x: x[0])
