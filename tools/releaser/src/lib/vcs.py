# coding: utf-8

import os
import sh
import time

from itertools import chain
from uuid import uuid4

from gitchronicler import chronicler

from tools.releaser.src.cli.utils import run_with_output


class VcsCtl(object):
    vcs = None
    master_branch = None

    def __init__(self, version=None, dry_run=None, *args, **kwargs):
        self.version = version or chronicler.get_current_version()
        self.message = 'releasing version ' + self.version
        self.dry_run = dry_run

    def run(self, *args):
        run_with_output(self.vcs, *args, _dry_run=self.dry_run)

    def add(self, path):
        self.run('add', path)

    def commit(self):
        self.run('commit', '-m', self.message)

    def delete_branch(self, branch):
        assert branch != self.master_branch
        try:
            self.run('branch', '-D', branch)
            return True
        except sh.ErrorReturnCode:
            return False

    def checkout(self, branch, create=False):
        if create:
            try:
                self.checkout(self.master_branch)
            except sh.ErrorReturnCode:
                pass
            self.delete_branch(branch)
            self.run('checkout', '-b', branch)
        else:
            self.run('checkout', branch)

    def pull(self):
        self.run('pull')

    def commit_command(self):
        self.add(chronicler.changelog_path)
        self.commit()

    def rollback_vcs_commit_command(self):
        pass

    def tag_command(self):
        pass

    def push_command(self):
        pass


class GitCtl(VcsCtl):
    vcs = 'git'
    master_branch = 'master'

    def __init__(self, remote=None, *args, **kwargs):
        self.remote = remote
        super(GitCtl, self).__init__(*args, **kwargs)

    def tag_command(self):
        self.run('tag', self.version)

    def push(self):
        self.run('push', self.remote)

    def push_tags(self):
        self.run('push', self.remote, '--tags')

    def rollback_vcs_commit_command(self):
        self.run('reset', '--soft', 'HEAD^')

    def push_command(self):
        self.push()
        self.push_tags()


class ArcCtl(VcsCtl):
    vcs = 'arc'
    master_branch = 'trunk'

    def __init__(self, direct_push=None, *args, **kwargs):
        self.direct_push = direct_push
        super(ArcCtl, self).__init__(*args, **kwargs)

    def push(self, remote):
        self.run('push', '-u', remote)

    def create_pr(self):
        run_with_output('ya', 'pr', 'create', '-A', '--wait', '-m', self.message, _dry_run=self.dry_run)

    def merge_pr(self):
        run_with_output('ya', 'pr', 'merge', '--now', '--force', '--wait', _dry_run=self.dry_run)

    def commit_command(self):
        if not self.direct_push:
            self.checkout('releaser-branch', create=True)
        super(ArcCtl, self).commit_command()

    def rollback_vcs_commit_command(self):
        if self.direct_push:
            # Could probably `arc reset --hard HEAD^`,
            # but that might be worse than doing nothing.
            return
        self.checkout('trunk')
        self.delete_branch('releaser-branch')

    def push_command(self):
        if self.direct_push:
            return self.run('push')

        remote_branch = 'users/{}/releaser-branch-{}'.format(os.getenv('LOGNAME'), str(uuid4()))
        self.push(remote_branch)
        self.create_pr()
        self.merge_pr()
        version = chronicler.get_current_version()
        self.rollback_vcs_commit_command()

        while True:
            # в trunk данные о новом релизе могут приехать не сразу
            # ждем пока не приедут
            time.sleep(3)
            self.pull()
            if self.has_recent_changes(version=version):
                break

    def has_recent_changes(self, version):
        return version == chronicler.get_current_version()


def get_vcs_ctl(*args, **kwargs):
    for ctl in (GitCtl, ArcCtl):
        try:
            getattr(sh, ctl.vcs).status()
        except (sh.ErrorReturnCode, sh.CommandNotFound):
            continue
        return ctl(*args, **kwargs)
