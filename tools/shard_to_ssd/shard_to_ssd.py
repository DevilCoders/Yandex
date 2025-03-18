#!/usr/bin/python

#
# Copy/delete shard to/from SSD with simple script.
# Use file locking on critical actions.
# Read README.md for details.
#
# Mikhail Kulemin <mkulemin@yandex-team.ru>
# Yandex 2016
#

import argparse
import os
import logging
import fcntl
import time
import shutil

# Global logger for module
logger = logging.getLogger("shard_to_ssd")
logger.addHandler(logging.NullHandler())


class LockException(Exception):
    ''' Exception when using locking. One can try to
        acquire lock once again.'''
    pass


class LockFatalException(Exception):
    '''Fatal error when using locking. It is not
       possible to use locking with current file'''
    pass


class FileLock():
    '''Implement locking based on files.'''
    def __init__(self, fname):
        self.fname = os.path.abspath(fname)
        self.locked = False

    def _try_lock(self, blocked=True):
        self.file = open(self.fname, "a+")

        if not blocked:
            try:
                fcntl.flock(self.file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            except IOError as ex:
                self.file.close()
                if ex.errno == os.errno.EWOULDBLOCK:
                    # Lock already acquired
                    logger.info("Can not acquire lock in non blocking mode.")
                    raise LockException
                else:
                    raise ex
        else:
            fcntl.flock(self.file.fileno(), fcntl.LOCK_EX)

        # After acquiring flock we need to check if file exists and inode
        # of locked file is the same as inode of file on fs.
        # If not - lock is not valid.
        try:
            if os.stat(self.fname).st_ino != os.fstat(self.file.fileno()).st_ino:
                self.file.close()
                logger.info("Inode of file and file descriptor are not the same. Trying to lock on wrong file.")
                raise LockException
        except IOError as ex:
            self.file.close()
            if ex.errno == os.errno.ENOENT:
                logger.info("Lock file unavailable after flock")
                raise LockException
            else:
                raise ex

        logger.info("Lock on file %s acquired" % self.fname)

    def get_file(self):
        if self.locked:
            self.file.seek(0)
            return self.file
        return None

    def lock(self, blocked=True):
        '''Acquire file lock'''
        if self.locked:
            return

        if blocked:
            while True:
                try:
                    self._try_lock(blocked)
                    break
                except LockException:
                    pass
                logger.info("Trying to acquire lock once again")
        else:
            self._try_lock(blocked)

        self.locked = True

    def unlock(self, delete=False):
        '''Release the lock and optionaly delete the  file'''
        if not self.locked:
            return

        if delete:
            os.unlink(self.fname)

        # lock is removed when fd closed
        self.file.close()
        logger.info("Lock on file %s released" % self.fname)


class Resource():
    '''Shared resource representation. File based locks placed in lockdir'''
    def __init__(self, worker_id, res_id, lockdir):
        if '\n' in worker_id:
            raise LockFatalException("Worker id has \\n - prohibited")
        self.worker_id = worker_id
        self.filelock = FileLock(os.path.join(lockdir,res_id))

    def lock(self):
        '''Acqure lock for resource and add to worker list. Resource is shared, but workers list can not be changed'''
        self.filelock.lock()
        f = self.filelock.get_file()
        workers = [l for l in f.read().split('\n') if l]
        if self.worker_id not in workers:
            workers.append(self.worker_id)
            f.truncate(0)
            f.write('\n'.join(workers))

    def workers(self):
        '''Return list of workers that used resource'''
        self.filelock.lock()
        f = self.filelock.get_file()
        workers = [l for l in f.read().split('\n') if l]
        return workers

    def free(self):
        self.filelock.lock()
        f = self.filelock.get_file()
        workers = [l for l in f.read().split('\n') if l]
        if self.worker_id in workers:
            workers.remove(self.worker_id)
            f.truncate(0)
            f.write('\n'.join(workers))
        if not workers:
            delete = True
        else:
            delete = False
        self.filelock.unlock(delete)

    def unlock(self):
        self.filelock.unlock()


# Class for manipulation with shards.
# Now it is very simple, but can be improved.
class Shard():
    '''Shard representation. Implementation is limited,
       but can be usefull in future'''
    def __init__(self, path):
        self.path = os.path.abspath(path)
        self.name = os.path.basename(self.path)

    def copy(self, dest):
        '''Copy shard to new directory'''
        shutil.copytree(self.path, os.path.join(dest, self.name))

    def check(self):
        '''Check shard integrity'''
        # Will be implemented later.
        pass

    def remove(self):
        '''Remove shard'''
        if not os.path.isdir(self.path):
            return
        shutil.rmtree(self.path)


def main():
    parser = argparse.ArgumentParser(description='Manipulate shards on ssd')
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-c", "--copy", action="store_true", help='Copy shard on ssh')
    group.add_argument("-r", "--remove", action="store_true", help='Remove shard from ssh if it is not needed for other services')

    parser.add_argument('shardpath', metavar='SHARD', type=str, help='Path to shard')
    parser.add_argument('-l', '--lockdir', metavar='DIR', type=str, help='Directory for placing lock files', default='/tmp')
    parser.add_argument('-s', '--ssddir', metavar='DIR', type=str, help='Mountpoint of ssd disk', default='/ssd')
    parser.add_argument('-i', '--id', metavar='NAME', type=str, help='Unique id of service', required=True)

    args = parser.parse_args()

    shardname = os.path.basename(args.shardpath)
    shard = Shard(args.shardpath)
    ssdshard = Shard(os.path.join(args.ssddir, shardname))
    resource = Resource(args.id, "%s.lock" % shardname, args.lockdir)

    if args.copy:
        resource.lock()
        if not os.path.exists(os.path.join(args.ssddir, shardname)):
            logging.info("Starting copy shard %s to ssd" % shardname )
            shard.copy(args.ssddir)
        else:
            logging.info("Shard %s is already on ssd" % shardname)
        resource.unlock()
    elif args.remove:
        resource.lock()
        workers = resource.workers()
        if len(workers) == 1:
            logging.info("I am last user. Removing shard %s from args.ssddir" % shardname)
            ssdshard.remove()
        else:
            logging.info("Some services need shard: %s Do nothing." % shardname)
        resource.free()
    else:
        logging.error("Specify at least one operation")


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    main()
