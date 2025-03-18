#!/usr/bin/env python

# WARNING: do not change this file without realizing locking algorithm

# BranchManager supports multithreading access.
# Implementation contains 3 basic locking models:
# (1) object protect their own data with "self.obj_lock"
# (2) there is per-branch lock for locking branch for any operation
# (3) there is a single open/close-branch branch for opening or closing branch
# when receive request to branch
#       if branch is not open acquire branch lock
#       if not enough free repo slots try to close existing tag first
#           to close tag we acquire this tag log, but without blocking to avoid deadlock
#           if we cannot acquire any tag (all are busy with another requests) we throw exception

import os
import sys
import time
import threading
import shutil
import tempfile
import xmlrpclib
import random
import requests
import urllib
import json

if __name__ == "__main__":
    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
    import gencfg

import config
import gaux.aux_utils
from gaux.aux_mongo import get_last_verified_commit
from core.svnapi import GENCFG_TRUNK_DATA_PATH, SvnRepository
from core.settings import SETTINGS
from threading import RLock
from core.db import DB, CURDB, set_current_thread_CURDB, get_real_db_object
from db_cacher import DBCacher

_debug = True


class DebugLockManager(object):
    def __init__(self, debug_mode):
        self.debug_file_name = "output.txt"
        self.debug_file_lock = RLock()
        self.debug_mode = debug_mode
        self.first_line = True

    def debug_print(self, _msg):
        if not self.debug_mode:
            return

        tid = self.__get_current_thread_id()
        msg = "[%s] %s" % (tid, _msg)

        self.debug_file_lock.acquire()
        try:
            with open(self.debug_file_name, 'a') as f:
                if self.first_line:
                    self.first_line = False
                    print >> f, "<<< new launch >>>"
                print >> f, msg
                f.close()
        finally:
            self.debug_file_lock.release()

    def __get_current_thread_id(self):
        return ('%s' % id(threading.currentThread()))[-4:]

    def create_lock(self, name):
        self.debug_print("created: %s" % name)
        return DebugLock(self, name)


class DebugLock(object):
    def __init__(self, parent, name):
        self.parent = parent
        self.name = name
        self.lock = RLock()

    def acquire(self, blocking=True):
        self.parent.debug_print("acquire: %s%s" % (self.name, ' non blocking' if not blocking else ''))
        result = self.lock.acquire(blocking=blocking)
        if not result:
            self.parent.debug_print("\tfailed to lock %s" % self.name)
        return result

    def release(self):
        self.parent.debug_print("release: %s" % self.name)
        self.lock.release()


GlobalLockManager = DebugLockManager(False)


# should allow:
#   - running parallel requests on different branches
#   - running 1 requests on a single branch depending on options
#   - opening branch with a parallel running requests to another branches
class BranchesLockManager(object):
    def __init__(self):
        self.obj_lock = GlobalLockManager.create_lock("obj lock (BranchesLockManager)")
        self.branches_locks = {}
        self.openclose_lock = GlobalLockManager.create_lock("open_close_lock")

    def __get_branch_lock(self, branch):
        self.obj_lock.acquire()
        try:
            if branch not in self.branches_locks:
                branch_lock = GlobalLockManager.create_lock("branch %s lock" % branch)
                self.branches_locks[branch] = branch_lock
            else:
                branch_lock = self.branches_locks[branch]
            return branch_lock
        finally:
            self.obj_lock.release()

    def acquire_branch_lock(self, branch, blocking=True):
        return self.__get_branch_lock(branch).acquire(blocking=blocking)

    def release_branch_lock(self, branch):
        return self.__get_branch_lock(branch).release()

    def acquire_openclose_branch_lock(self, blocking=True):
        return self.openclose_lock.acquire(blocking=blocking)

    def release_openclose_branch_lock(self):
        return self.openclose_lock.release()


class Branch(object):
    def __init__(self, name):
        self.name = name
        self.is_opened = False
        self.farm_id = None
        self.farm_path = None
        self.last_commit = None
        SvnRepository(farm_path).get_last_commit()

    def get_last_commit(self):
        assert self.is_opened
        return self.last_commit

    def open(self, farm_id, farm_path, is_readonly):
        self.farm_id = farm_id
        self.farm_path = farm_path
        self.last_commit = SvnRepository(farm_path).get_last_commit()
        self.is_opened = True
        self.is_readonly = is_readonly

    def close(self):
        self.is_opened = False
        self.farm_id = None
        self.farm_path = None
        self.last_commit = None

    def get_gencfg_name(self):
        return self.name.get_gencfg_name()

    def get_repo_name(self):
        return self.name.get_repo_name()

    def is_opened(self):
        return self.is_opened


# open branch

# close branch

# update recent copy

# get data

# [recent copy]

# [tag-1]

# [tag-2]

# [tag 3]

# [open-close operation]

# read existing tag

# open-close-update


class BranchesRepoManager(object):
    def __init__(self, farm_size):
        self.obj_lock = GlobalLockManager.create_lock("obj lock (BranchesRepoManager)")

        self.db_farm_path = os.path.abspath(os.path.join(config.MAIN_DIR, "db_new", "farm"))
        self.db_links_path = os.path.abspath(os.path.join(config.MAIN_DIR, "db_new", "links"))
        self.db_recent_copy_path = os.path.abspath(os.path.join(config.MAIN_DIR, "db_new", "recent-copy"))
        self.db_temp = os.path.abspath(os.path.join(config.MAIN_DIR, "db_new", "temp"))

        self.farm_size = farm_size
        self.farms = set()
        self.tags = set()

        self.__reset()

    def update_remote(self):
        self.__update_recent_copy()
        repo = SvnRepository(self.db_recent_copy_path)

        self.obj_lock.acquire()
        try:
            self.__update_tag_names(repo)
        finally:
            self.obj_lock.release()

            # repo = SvnRepository(self.db_recent_copy_path)
            # new_tags = self.__get_tags(repo)
            # new_tags = {new_tag.gencfg_name : new_tag for new_tag in new_tags}
            # rotten_tags = [old_tag for old_tag in ]
            # for old_tag in self.old_tags.items():
            #    if not old_tag.is_opened():
            #        continue
            #    rotten_tags =

            # TODO: update opened tag commits
            # TODO: close rotten tags

    def __clone_branch_atomic(self, path, branch):
        temp_path = tempfile.mkdtemp(dir=self.db_temp)

        try:
            repo = SvnRepository.clone(temp_path, "db")
            repo.checkout(branch)
        except Exception:
            gaux.aux_utils.rm_tree(temp_path)
            raise
        os.rename(temp_path, path)

    def __update_recent_copy(self):
        repo = SvnRepository(self.db_recent_copy_path)
        repo.sync()

    def __update_tag_names(self, repo):
        self.tags = set()
        for tagname in repo.tags():
            self.tags.add(tagname)

    def __get_farm_path(self, n):
        return os.path.join(self.db_farm_path, "%s" % n)

    def __build_recent_copy(self):
        assert (not os.path.exists(self.db_recent_copy_path))
        self.__clone_branch_atomic(self.db_recent_copy_path, "trunk")

    def __reset(self):
        if _debug:
            print "Initializing repos..."

        if not os.path.exists(self.db_farm_path):
            gaux.aux_utils.mkdirp(self.db_farm_path)
        if not os.path.exists(self.db_links_path):
            gaux.aux_utils.mkdirp(self.db_links_path)
        if not os.path.exists(self.db_temp):
            gaux.aux_utils.mkdirp(self.db_temp)

        # precaching recent copy
        if not os.path.exists(self.db_recent_copy_path):
            if _debug:
                print "Cloning recent copy..."
            self.__build_recent_copy()
        else:
            if _debug:
                print "Updating recent copy..."
            self.__update_recent_copy()

        # update tag names
        self.__update_tag_names(SvnRepository(self.db_recent_copy_path))

        # update farm
        if _debug:
            print "Updating farm..."
        for i in range(self.farm_size):
            farm_path = self.__get_farm_path(i)
            if not os.path.exists(farm_path):
                # do local copy, not remote clone
                farm_path_tmp = farm_path + ".tmp"
                if os.path.exists(farm_path_tmp):
                    gaux.aux_utils.rm_tree(farm_path_tmp)

                shutil.copytree(self.db_recent_copy_path, farm_path_tmp)
                os.rename(farm_path_tmp, farm_path)
        self.farms = set([self.__get_farm_path(i) for i in range(self.farm_size)])

        # read existing links to fill the table
        # and check if they are valid
        # ** but skip this step for speed


        def __process_link(base_dir, link_name):
            link_path = os.path.abspath(os.path.join(base_dir, link_name))

            if not os.path.islink(link_path):
                return False, "is not link"

            farm_path = os.readlink(link_path)
            if not os.path.isabs(farm_path):
                farm_path = os.path.abspath(os.path.join(base_dir, farm_path))

            try:
                repo = SvnRepository(farm_path)
            except Exception as err:
                return False, "could not read repo or detect current branch: %s" % err

            if os.path.dirname(farm_path) != self.db_farm_path:
                return False, "link points to invalid location %s" % farm_path

            farm_index = os.path.basename(farm_path)

            try:
                farm_index = int(farm_index)
            except Exception:
                return False, "invalid link name \"%s\"" % farm_index

            if farm_index >= self.farm_size:
                return False, "farm index \"%s\" is too big" % farm_index

            return True, farm_path

        opened_branches = {}
        for link_name in os.listdir(self.db_links_path):
            link_path = os.path.join(self.db_links_path, link_name)
            result, farm_or_err = __process_link(self.db_links_path, link_name)
            if not result:
                print >> sys.stderr, "Warning: invalid link %s: %s" % \
                                     (link_path, farm_or_err)
                print >> sys.stderr, "Link will be removed"
                if os.path.islink(link_path):
                    os.unlink(link_path)
                elif os.path.isdir(link_path):
                    gaux.aux_utils.rm_tree(link_path)
                else:
                    os.unlink(link_path)
                continue
            opened_branches[link_name] = farm_or_err
        self.opened_branches = opened_branches

        if _debug:
            print "Done initializing repos..."

    def __get_branch_link_path(self, branch):
        return os.path.join(self.db_links_path, branch)

    def hard_reset(self):
        if _debug:
            print "Hard reseting..."

        self.obj_lock.acquire()
        try:
            if os.path.exists(self.db_recent_copy_path):
                gaux.aux_utils.rm_tree(self.db_recent_copy_path)
            if os.path.exists(self.db_farm_path):
                gaux.aux_utils.rm_tree(self.db_farm_path)
            if os.path.exists(self.db_links_path):
                gaux.aux_utils.rm_tree(self.db_links_path)
            if os.path.exists(self.db_temp):
                gaux.aux_utils.rm_tree(self.db_temp)

            self.__reset()
        finally:
            self.obj_lock.release()

    def get_opened_branches(self):
        self.obj_lock.acquire()
        try:
            return self.opened_branches.keys()
        finally:
            self.obj_lock.release()

    def get_max_number_of_opened_branches(self):
        self.obj_lock.acquire()
        try:
            return self.farm_size
        finally:
            self.obj_lock.release()

    def open_branch(self, branch):
        # TODO: don't access branch! use recent copy!

        if _debug:
            print "Opening branch %s..." % branch

        # we assume that open/close operations are running simultaneously only by a single thread
        # that is why we can assume that self.opened_branches doesn't change

        self.obj_lock.acquire()
        try:
            if branch in self.opened_branches:
                raise Exception("Branch %s is already opened" % branch)

            if len(self.get_opened_branches()) >= self.get_max_number_of_opened_branches():
                raise Exception("Cannot open branch %s: no room for new branch" % branch)

            free_farms = set(self.farms) - set(self.opened_branches.values())
            assert (len(free_farms) == self.farm_size - len(self.opened_branches))

            farm = list(free_farms)[0]
        finally:
            self.obj_lock.release()

        # warning: can take some time!
        repo = SvnRepository(farm)
        repo.sync()

        self.obj_lock.acquire()
        try:
            self.__update_tag_names(repo)
            repo_branch_name = "%s" % branch
        finally:
            self.obj_lock.release()

        repo.checkout(repo_branch_name)

        self.obj_lock.acquire()
        try:
            os.symlink(farm, os.path.join(self.db_links_path, branch))
            self.opened_branches[branch] = farm
        finally:
            self.obj_lock.release()

        if _debug:
            print "Done opening branch %s" % branch

    def close_branch(self, branch):
        if _debug:
            print "Closing branch %s..." % branch

        self.obj_lock.acquire()
        try:
            assert (branch in self.opened_branches)
            os.unlink(os.path.join(self.db_links_path, branch))
            del self.opened_branches[branch]
        finally:
            self.obj_lock.release()

        if _debug:
            print "Done closing branch %s" % branch

    def is_branch_opened(self, branch):
        self.obj_lock.acquire()
        try:
            return branch in self.opened_branches
        finally:
            self.obj_lock.release()

    def get_branch_db_path(self, branch):
        self.obj_lock.acquire()
        try:
            assert (branch in self.opened_branches)
            return self.opened_branches[branch]
        finally:
            self.obj_lock.release()


class BranchesStats(object):
    def __init__(self, opened_branches):
        # TODO: add information about number of branch open/close operations
        self.branches_last_request_time = {x: time.time() for x in opened_branches}
        self.obj_lock = GlobalLockManager.create_lock("obj lock (BranchesStats)")

    def sort_branches(self):
        self.obj_lock.acquire()
        try:
            result = sorted(self.branches_last_request_time.keys(), cmp= \
                lambda x, y: cmp(self.branches_last_request_time[x], self.branches_last_request_time[y]))
            return result
        finally:
            self.obj_lock.release()

    def touch_branch(self, branch):
        self.obj_lock.acquire()
        try:
            self.branches_last_request_time[branch] = time.time()
        finally:
            self.obj_lock.release()


class BranchDBs(object):
    def __init__(self, branch_dbs):
        self.obj_lock = GlobalLockManager.create_lock("obj lock (BranchDBs)")
        self.branch_dbs = {}
        for branch, db_repo_path in branch_dbs.items():
            self.get_branch_db(branch, db_repo_path)

    def __create_branch_db(self, branch, branch_repo_path):
        result = DB(branch_repo_path)
        if branch != "trunk":
            result.set_readonly(True)
        return result

    def get_branch_db(self, branch, branch_repo_path):
        self.obj_lock.acquire()
        try:
            if branch not in self.branch_dbs:
                branch_db = self.__create_branch_db(branch, branch_repo_path)
                self.branch_dbs[branch] = branch_db
            return self.branch_dbs[branch]
        finally:
            self.obj_lock.release()

    def free_branch_db(self, branch):
        self.obj_lock.acquire()
        try:
            assert (branch in self.branch_dbs)
            del self.branch_dbs[branch]
        finally:
            self.obj_lock.release()


class TagsDBManager(object):
    """
        :param parent: parent object (WebApiBase or something similar)
        :param max_simultaneous_dbs: maximum number of simultaneously opened dbs (number of dirs in db_new/farm)
    """

    def __init__(self, parent, max_simultaneous_dbs):
        self.parent = parent
        self.repo_man = BranchesRepoManager(max_simultaneous_dbs)
        self.lock_man = BranchesLockManager()
        self.stats = BranchesStats(self.repo_man.get_opened_branches())
        branch_paths = {branch: self.repo_man.get_branch_db_path(branch) for branch in
                        self.repo_man.get_opened_branches()}
        self.dbs = BranchDBs(branch_paths)
        self.obj_lock = GlobalLockManager.create_lock("obj lock (TagsDBManager)")
        set_current_thread_CURDB(None)
        self.tags_cacher = DBCacher()

        # cached sandbox data (thanks to GIL, we can use it safely in all threads)
        self.sandbox_released_tags = set()
        self.sandbox_last_released_tag = None
        self.sandbox_last_released_tag_cached_at = None

        self.db_updater_thread = threading.Thread(target = self.update_db)
        self.db_updater_thread.daemon = True

    def begin_request(self, request):
        """
            Prepare repository for specified tag. Check if we already checked out our tag in db_new/farm. If not, drop one of tags from this
            directory and checkout requested tag

            :param request: flast Request object
            :return: None
        """

        branch = request.branch

        # check if specified tag already released in sandbox
        if branch not in self.sandbox_released_tags:
            attrs_param = {'tag': branch}
            attrs_param = urllib.quote(json.dumps(attrs_param))

            url = '{}/resource?limit=1&type=CONFIG_GENERATOR&state=READY&attrs={}'.format(SETTINGS.services.sandbox.rest.url, attrs_param)

            resources = requests.get(url).json()['items']

            if len(resources) == 0:
                raise Exception("Tag <%s> not released in sandbox" % branch)
            if resources[0]['attributes'].get('released', None) != 'stable':
                raise Exception("Tag <%s> not released in sandbox" % branch)
            self.sandbox_released_tags.add(branch)

        self.lock_man.acquire_branch_lock(branch)

        # ** if here we will throw an exception by definition we will call end_request()
        #       where lock will be released
        # now we have exclusive access to this branch

        was_opened = self.repo_man.is_branch_opened(branch)
        if not was_opened:
            self.__prepare_branch_db(branch)

        self.stats.touch_branch(branch)
        branch_repo_path = self.repo_man.get_branch_db_path(branch)
        branch_db = self.dbs.get_branch_db(branch, branch_repo_path)
        set_current_thread_CURDB(branch_db)

        if not was_opened:
            # copy cache or create and save
            is_cache_restored = self.tags_cacher.try_restore_cache(CURDB, branch)
            if not is_cache_restored:
                CURDB.precalc_caches()
                self.tags_cacher.try_save_cache(CURDB, branch)

    def __prepare_branch_db(self, prep_branch):
        # assuming we already acquired lock for this branch
        self.lock_man.acquire_openclose_branch_lock()
        try:
            if self.repo_man.is_branch_opened(prep_branch):
                return

            n_opened = len(self.repo_man.get_opened_branches())
            max_n_opened = self.repo_man.get_max_number_of_opened_branches()
            if n_opened >= max_n_opened:
                could_close_branch = False
                ordered_branches = self.stats.sort_branches()
                for close_branch in ordered_branches:
                    has_lock = self.lock_man.acquire_branch_lock(close_branch, blocking=False)
                    if not has_lock:
                        continue
                    if not self.repo_man.is_branch_opened(close_branch):
                        self.lock_man.release_branch_lock(close_branch)
                        continue

                    self.parent.app.logger.info(
                        "[%s]\t[CORE]\t[TAGS_DB]\tClosing branch %s in order to open branch %s" % (
                            self.parent.make_time_str(), close_branch, prep_branch))

                    self.dbs.free_branch_db(close_branch)
                    self.repo_man.close_branch(close_branch)
                    self.lock_man.release_branch_lock(close_branch)
                    could_close_branch = True
                    break

                if not could_close_branch:
                    raise Exception("All branches slots are currenly busy, ask system administrator for help")

            self.parent.app.logger.info(
                "[%s]\t[CORE]\t[TAGS_DB]\tOpening branch %s" % (self.parent.make_time_str(), prep_branch))
            self.repo_man.open_branch(prep_branch)
        finally:
            self.lock_man.release_openclose_branch_lock()

    def end_request(self, branch):
        try:
            set_current_thread_CURDB(None)
            self.stats.touch_branch(branch)
        finally:
            self.lock_man.release_branch_lock(branch)

    def get_recent_branch(self):
        """
            Get last released tag. Use cached data in order to decrease rps to sandbox
        """

        now = time.time()
        if self.sandbox_last_released_tag is None or now > self.sandbox_last_released_tag_cached_at + 100:
            attrs_param = {'released': 'stable'}
            attrs_param = urllib.quote(json.dumps(attrs_param))

            url = '{}/resource?limit=1&type=CONFIG_GENERATOR&state=READY&attrs={}'.format(SETTINGS.services.sandbox.rest.url, attrs_param)

            try:
                resource = requests.get(url).json()['items'][0]
            except Exception as e:  # do not raise exception when can not read last tags from sandbox
                if self.sandbox_last_released_tag is None:
                    raise

            self.sandbox_last_released_tag = resource['attributes']['tag']
            self.sandbox_last_released_tag_cached_at = now

        return self.sandbox_last_released_tag

    def hard_reset(self):
        # !!! UNSAFE
        self.repo_man.hard_reset()

    def update_db(self):
        while True:
            print "Start update cycle"
            self.lock_man.acquire_openclose_branch_lock()
            try:
                self.repo_man.update_remote()
            finally:
                self.lock_man.release_openclose_branch_lock()
            time.sleep(1)

class TDbUpdater(object):
    """
        Object of this class represents entity, updating db and setting curdb to new db object
    """

    def __init__(self, parent, farm_path, sync_func, set_db_func):
        """
            Initialize first time.

            :type parent: web_shared.api_base.WebApiBase
            :type farm_path: str
            :type sync_func: (str) -> bool
            :type set_db_func: () -> ()

            :param parent: root object
            :param farm_path: path to directory with syncing repos
            :param sync_func: func, which syncs specified directory to last commit
            :param set_db_func: func, updating curdb with generated here db object
        """
        self.parent = parent
        self.sync_func = sync_func
        self.set_db_func = set_db_func

        # temporary database dirs to sync to
        self.db_sync_path1 = os.path.join(os.path.abspath(farm_path), "db1")
        self.db_sync_path2 = os.path.join(os.path.abspath(farm_path), "db2")
        for sync_path in [self.db_sync_path1, self.db_sync_path2]:
            repo = SvnRepository.clone(sync_path, "db")
            self.sync_func(sync_path)
            # self.log("Directory <%s> initialized to commit <%s>" % (
            #     sync_path, repo.get_last_commit_id()))
            self.last_commit = repo.get_last_commit_id()

        # update curdb
        self.set_db_func(DB(self.db_sync_path1))
        self.db_sync_path = self.db_sync_path2
        # self.log("Set curdb to directory <%s>" % sync_path)

        self.finished = False

    def sync(self):
        """
            Check if we have new commits, and update db if have ones
        """

        self.sync_func(self.db_sync_path)

        new_last_commit = DB(self.db_sync_path).get_repo().get_last_commit_id()
        if new_last_commit == self.last_commit:
            self.log("Directory <%s> has not changed (last commit <%s>)" % (self.db_sync_path, self.last_commit))
            return

        # have some changes, update
        self.log("Directory <%s> updating: <%s> -> <%s>" % (
            self.db_sync_path, self.last_commit, new_last_commit))
        real_db_object = DB(self.db_sync_path)
        real_db_object.precalc_caches()
        self.set_db_func(real_db_object)
        self.log("CURDB updated to commit <%s>, path <%s>" % (new_last_commit, self.db_sync_path))

        self.last_commit = new_last_commit
        if self.db_sync_path == self.db_sync_path1:
            self.db_sync_path = self.db_sync_path2
        else:
            self.db_sync_path = self.db_sync_path1

    def run(self):
        """
            Inifinte cycle of updating
        """

        self.log("Started")

        while not self.finished:
            self.log("Syncing...")
            try:
                self.sync()
            except Exception:
                pass
            time.sleep(1)

        self.log("Finished")

    def log(self, message):
        self.parent.app.logger.info("[%s]\t[CORE]\t[DBUPDATER]\t%s" % (
            self.parent.make_time_str(), message))

class UnstableDBManager(object):
    """
        Manager hold trunk database and update to last commit when requested
    """

    def __init__(self, parent, trunk_db_path):
        self.parent = parent
        self.obj_lock = GlobalLockManager.create_lock("obj lock (UnstableDBManager)")

        if self.parent.options.enable_auto_update:
            if trunk_db_path is None:
                trunk_db_path = config.MAIN_DB_DIR

            self.db_updater = TDbUpdater(
                self.parent, os.path.dirname(trunk_db_path), self.sync_to_last_commit,
                self.set_new_curdb)
            self.db_updater_thread = threading.Thread(target = self.db_updater.run)
            self.db_updater_thread.daemon = True
        else:
            if trunk_db_path is None:
                self.real_db_object = get_real_db_object()
            else:
                self.real_db_object = DB(trunk_db_path)

    def begin_request(self, request):
        branch = request.branch

        if branch != "unstable":
            raise self.parent.GencfgNotFoundError("Invalid request to unsupported branch <%s>" % branch)

        # check if we have to wait until updated to specified commit
        if 'commit' in request.args:
            required_commit = int(request.args['commit'])
            found = False
            for i in range(10):
                self.obj_lock.acquire()
                try:
                    last_commit = self.real_db_object.get_repo().get_last_commit_id()
                    if last_commit >= required_commit:
                        found = True
                        break
                finally:
                    self.obj_lock.release()
                time.sleep(1)

            if not found:
                raise Exception("Unabled to update db to commit <%s>: current commit is <%s>" % (required_commit, self.real_db_object.get_repo().get_last_commit_id()))

        self.obj_lock.acquire()
        set_current_thread_CURDB(self.real_db_object)

    def end_request(self, branch):
        set_current_thread_CURDB(None)
        if branch != "unstable":
            raise self.parent.GencfgNotFoundError("Invalid request to unsupported branch <%s>" % branch)
        self.obj_lock.release()

    def sync_to_last_commit(self, path):
        changed, commits_discarded = DB(path).sync()
        return changed
    def set_new_curdb(self, new_real_db_object):
        try:
            self.obj_lock.acquire()
            self.real_db_object = new_real_db_object
            set_current_thread_CURDB(self.real_db_object)
        finally:
            self.obj_lock.release()


class TrunkDBManager(object):
    """
        Manager hold trunk database and update to last VERIFIED commit when requested
    """

    def __init__(self, parent, trunk_db_path):
        self.parent = parent
        self.obj_lock = GlobalLockManager.create_lock("obj lock (TrunkDBManager)")

        self.switch_to_commit_at = dict()  # dict with timings for switch to commit

        if self.parent.options.enable_auto_update:
            if trunk_db_path is None:
                trunk_db_path = config.MAIN_DB_DIR

            self.db_updater = TDbUpdater(
                self.parent, os.path.dirname(trunk_db_path), self.sync_to_last_commit,
                self.set_new_curdb)
            self.db_updater_thread = threading.Thread(target = self.db_updater.run)
            self.db_updater_thread.daemon = True
        else:
            if trunk_db_path is None:
                self.real_db_object = get_real_db_object()
            else:
                self.real_db_object = DB(trunk_db_path)

        self.real_db_object.set_readonly(True)  # trunk is read only now

        # cache some data
        # self.real_db_object.fast_check()

    def begin_request(self, request):
        branch = request.branch

        if branch != "trunk":
            raise self.parent.GencfgNotFoundError("Invalid request to unsupported branch <%s>" % branch)
        self.obj_lock.acquire()
        set_current_thread_CURDB(self.real_db_object)

    def end_request(self, branch):
        set_current_thread_CURDB(None)
        if branch != "trunk":
            raise self.parent.GencfgNotFoundError("Invalid request to unsupported branch <%s>" % branch)
        self.obj_lock.release()

    def sync_to_last_commit(self, path):
        last_verified_commit = get_last_verified_commit()
        if last_verified_commit not in self.switch_to_commit_at:
            self.switch_to_commit_at[last_verified_commit] = int(time.time()) + random.randrange(1200)

        now = time.time()
        good_commits = [x for (x, y) in self.switch_to_commit_at.iteritems() if y < now]
        if not good_commits:
            return False

        switch_to_commit = max(good_commits)
        if switch_to_commit < DB(path).get_repo().get_last_commit_id():
            return False

        changed, commits_discarded = DB(path).sync(commit=switch_to_commit)
        return changed

    def set_new_curdb(self, new_real_db_object):
        try:
            self.obj_lock.acquire()
            self.real_db_object = new_real_db_object
            set_current_thread_CURDB(self.real_db_object)
        finally:
            self.obj_lock.release()
