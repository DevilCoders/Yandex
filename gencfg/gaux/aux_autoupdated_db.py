"""Class, representing autoupdated gencfg db"""

import os
import threading
import time
import shutil

from core.db import DB
from gaux.aux_mongo import get_last_verified_commit
from core.svnapi import GENCFG_TRUNK_DATA_PATH, SvnRepository


class TRepoUpdater(object):
    """Repo updater"""

    __slots__ = (
        'debug',  # debug flag
        'finished',  # flag to finish autoupdate
        'farm_path',  # farm path
        'template_repo',  # template repo (new repos are created by copying template repo)
        'last_commit_func',  # func to get new commit to switch
        'cur_commit',  # current commit
        'cur_db',  # current db object
    )

    def __init__(self, farm_path, last_commit_func, debug=False):
        """Initialize

        :param farm_path: directory with checkouted dbs
        :type farm_path: str
        :param last_commit_func: function, returning commit id we need to sync to
        :type last_commit_func: python func"""

        self.debug = debug

        self.log('Initializing repo updater')

        self.finished = False

        # initialize farm dir
        self.farm_path = farm_path
        if not os.path.exists(self.farm_path):
            os.makedirs(self.farm_path)

        # initialize repos
        self.log('Cloning repositories')
        template_path = os.path.join(self.farm_path, 'template')
        if os.path.exists(template_path):
            if SvnRepository(template_path).svn_info_relative_url() != '^/trunk/data/gencfg_db':
                self.log('Cloning template at <{}>'.format(template_path))
                shutil.rmtree(template_path)
                SvnRepository.clone(template_path, GENCFG_TRUNK_DATA_PATH)
        else:
            SvnRepository.clone(template_path, GENCFG_TRUNK_DATA_PATH)
        self.template_repo = SvnRepository(template_path)

        # sync path
        self.log('Syncing to last commit')
        self.last_commit_func = last_commit_func
        self.cur_commit = None
        self.cur_db = None
        self.sync()

        self.log('Repo updater initialization finished')

    def sync(self):
        """Check if we have new commits and sync next repo to this commit"""
        new_commit = self.last_commit_func(self.template_repo)
        if new_commit == self.cur_commit:
            self.log('Last commit not changed ({})'.format(self.cur_commit))
            return self.db

        new_repo_path = os.path.join(self.farm_path, 'commit_{}'.format(new_commit))
        if os.path.exists(new_repo_path):
            shutil.rmtree(new_repo_path)

        # sync repo
        self.log('Updating last commit: {} -> {} , set dir to {}'.format(self.cur_commit, new_commit, new_repo_path))
        self.template_repo.sync(new_commit)
        DB(self.template_repo.path).precalc_caches()
        shutil.copytree(self.template_repo.path, new_repo_path)
        new_db = DB(new_repo_path, temporary=True)
        new_db.groups.get_groups()

        # switch to new db
        self.cur_commit = new_commit
        self.cur_db = new_db
        self.log('Db updated to commit {} (path {})'.format(self.cur_commit, new_repo_path))

    def run(self):
        """Infinit loop of db updating"""

        self.log('Started')

        while not self.finished:
            self.log('Syncing...')
            try:
                self.sync()
            except Exception:
                pass
            time.sleep(1)

    def log(self, msg):
        if self.debug:
            print '[{}] [TRepoUpdater] {}'.format(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime()), msg)


class TAutoupdatedDb(object):
    """Class, representing autoudated db"""

    class EDbType(object):
        UNSTABLE = 'unstable'
        TRUNK = 'trunk'

    def __init__(self):
        pass  # lazy initialization

    def start(self, farm_path, db_type, debug=False):
        """Initialize updater"""

        if db_type == TAutoupdatedDb.EDbType.UNSTABLE:
            last_commit_func = self.unstable_last_commit
        elif db_type == TAutoupdatedDb.EDbType.TRUNK:
            last_commit_func = self.trunk_last_commit

        self.repo_updater = TRepoUpdater(farm_path, last_commit_func, debug=debug)

        # run in thread
        self.repo_updater_thread = threading.Thread(target=self.repo_updater.run)
        self.repo_updater_thread.daemon = True
        self.repo_updater_thread.start()

    def get_db(self):
        return self.repo_updater.cur_db

    def finish(self):
        self.repo_updater.finished = True
        self.repo_updater_thread.join()

    @staticmethod
    def unstable_last_commit(repo):
        """Get last commit for unstable

        :param repo: repo object
        :type repo: core.svnapi.SvnRepository"""

        return repo.get_last_commit_id(url=repo.svn_info_relative_url())

    @staticmethod
    def trunk_last_commit(repo):
        return get_last_verified_commit()
