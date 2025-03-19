from base_controller import BaseController
from misc import log
from time import sleep
import re


class MongodController (BaseController):

    def __init__(self, host):
        super(MongodController, self).__init__(host, 27018, '/etc/mongodb.conf', 'mongodb')

    @log
    def clean_datadir(self):
        dbpath = self.config['dbpath']

        # need at least two slashes to consider dbpath correct
        if not re.match(r'^/.+/', dbpath):
            raise RuntimeError('There is something wrong with datadir on %s: %s' % self.host, dbpath)

        self.ssh.run('rm -rf %s/*' % dbpath)

    @log
    def rename_rs(self, name):
        coll = self.mongo.local['system.replset']
        data = coll.find_one()
        data['_id'] = name
        coll.remove({})
        coll.insert(data)

        self.config['replSet'] = name
        self.write_config()
        self.restart()

    @log
    def copy_from(self, remote_source):
        tmp = '/var/tmp/backup_restore.tar.gz'
        self.ssh.run('rm -f %s' % tmp)
        self.ssh.run('rsync -avH %s %s' % (remote_source, tmp))
        self.ssh.run('pigz -dc %s | tar x -C %s' % (tmp, '/'.join(self.config['dbpath'].split('/')[:-1])))

    @log
    def init_rs(self, hostlist):
        self.stop()
        self.start()
        self.connect()
        db = self.mongo['local']
        db['system.replset'].remove({})
        self.restart()

        self.connect()
        db = self.mongo['admin']
        db.command('replSetInitiate')

        while db.command('replSetGetStatus')['myState'] != 1:  # == PRIMARY
            sleep(1)

        # new config
        members = []
        inc = 0
        for h in hostlist:
            members.append({'_id': inc, 'host':'%s:27018' % h})

        conf = db.command('replSetGetConfig')['config']
        conf['members'] = members
        db.command({'replSetReconfig': conf, 'force': True})

        self.mongo['local']['system.replset'].update({}, {'$set': {'version': 1}})
        self.stop()
