"""
    Tests svnapi module. DOES NOT WORK YET (made from test gitapi module).
"""

from __future__ import unicode_literals

import os
import traceback
from multiprocessing.pool import ThreadPool

import pytest

unstable_only = pytest.mark.unstable_only

import gaux.aux_utils

from core.svnapi import SvnRepository, PushRejectedError, ChangesDiscardedOnMerge

SERVER_REPO_URL = None


@unstable_only
@pytest.mark.parametrize("concurrency", [1, 5])
def test_svnapi_update(temp_dir, concurrency):
    commit_count = 50

    server_repo_url = _prepare_server_repo(temp_dir)
    control_repo = _prepare_control_repo(temp_dir, server_repo_url)

    repos = {}
    for repo_id in xrange(concurrency):
        repos[repo_id] = SvnRepository.clone(server_repo_url, os.path.join(temp_dir, "{}.svn".format(repo_id)))

    def process_repo(repo_id):
        try:
            _test_commit_changes(repo_id, repos[repo_id], commit_count, commit_count * (concurrency - 1))
        except:
            traceback.print_exc()
            raise

    pool = ThreadPool(processes=len(repos))
    pool.map(process_repo, repos)

    control_repo.update_remote()
    stdout, stderr = _git_cmd(control_repo.path, ["log", "--all", "--format=%s"])
    assert sorted(stdout.strip().split("\n")) == sorted(
        ["{}: {}".format(repo_id, data_id) for repo_id in repos for data_id in xrange(commit_count)] +
        ["Initial commit"])


def _prepare_server_repo(temp_dir):
    if SERVER_REPO_URL is not None:
        return SERVER_REPO_URL

    repo_path = os.path.join(temp_dir, "server.svn")
    os.mkdir(repo_path)
    _git_cmd(repo_path, ["init", "--bare"])

    return repo_path


def _prepare_control_repo(temp_dir, server_repo_url):
    repo_path = os.path.join(temp_dir, "control.svn")
    os.mkdir(repo_path)
    _git_cmd(repo_path, ["init"])
    _git_cmd(repo_path, ["remote", "add", "origin", server_repo_url])

    repo = SvnRepository(repo_path)
    with open(os.path.join(repo_path, "data"), "w") as data_file:
        data_file.write(b"data")
    repo.add(["data"])
    repo.commit("Initial commit")
    repo.push(force=True)

    return repo


def _test_commit_changes(repo_id, repo, count, max_rejects):
    rejects = 0

    for data_id in xrange(count):
        while rejects <= max_rejects:
            data = "{}: {}".format(repo_id, data_id)
            with open(os.path.join(repo.path, "data"), "w") as data_file:
                data_file.write(data)

            repo.commit(data, ["data"])

            try:
                repo.push(lock_retry_timeout=0.1)
            except PushRejectedError:
                try:
                    repo.sync(force=True)
                except ChangesDiscardedOnMerge:
                    pass

                rejects += 1
            else:
                break
        else:
            raise Exception("Too many rejects.")


def _git_cmd(path, args):
    return gaux.aux_utils.run_command(["git"] + args, cwd=path, close_fds=True)[1:]
