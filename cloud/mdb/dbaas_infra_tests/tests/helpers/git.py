#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Git-helpers for dbaas infra tests
"""

import getpass
import json
import logging
import os
import shutil
import subprocess
from distutils import dir_util

from retrying import retry

from . import utils

TWO_BRANCH_PROJECTS = [
    'mdb/dbaas-internal-api-image',
    'mdb/dbaas-infrastructure-test',
    'mdb/dbaas-metadb',
    'mdb/dbaas-worker',
]


def gerrit_refspec(project, base, branch, topic):
    """
    Connect to gerrit via ssh and query refspec
    """
    if base is None or topic is None:
        raise RuntimeError('No info for getting refspec: ' 'base:{base}, topic:{topic}'.format(base=base, topic=topic))
    host, address = base.replace('ssh://', '').split(':', 1)
    port, *_ = address.partition('/')
    cmd = [
        'ssh',
        '-p',
        port,
        host,
        'gerrit',
        'query',
        '--current-patch-set',
        '--format=JSON',
        'status:open',
        'topic:%s' % topic,
        'branch:%s' % branch,
        'project:%s' % project,
    ]
    output = subprocess.check_output(cmd)
    # Gerrit has no order by, so we sort here
    data = [json.loads(x.decode('utf-8')) for x in output.splitlines() if x]
    logging.debug('raw gerrit data: %s', json.dumps(data))
    sorted_data = sorted(data, key=lambda d: int(d.get('number', 0)))

    if 'currentPatchSet' not in sorted_data[-1]:
        if branch != 'master' and project not in TWO_BRANCH_PROJECTS:
            return gerrit_refspec(project, base, 'master', topic)
        raise RuntimeError('No patchset for topic %s' % topic)

    return sorted_data[-1]['currentPatchSet']['ref']


def checkout_gerrit_topic(topic, path, project, repo_url, gerrit_url=None, topic_refspec=None):
    """
    Checkout gerrit topic if exists
    """
    target_branch = os.environ.get('GERRIT_BRANCH', 'master')

    # Resolve topic refspec if not provided.
    if topic_refspec is None:
        try:
            topic_refspec = gerrit_refspec(project, gerrit_url, target_branch, topic)
        except RuntimeError as err:
            logging.warning('refspec retrival failed: %s', err)
            return

    target_branch_spec = 'origin/{branch}'.format(branch=target_branch)
    git(['fetch', repo_url, topic_refspec], cwd=path)
    git(['checkout', 'FETCH_HEAD', '-b', 'current'], cwd=path)
    try:
        git(['rev-parse', '--verify', target_branch_spec], cwd=path)
    except Exception:
        git(['rebase', 'origin/master', 'current'], cwd=path)
        return

    git(['rebase', target_branch_spec, 'current'], cwd=path)


def checkout_git_branch(branch, path):
    """
    Checkout git branch if exists
    """
    try:
        git(['checkout', branch], cwd=path)
    except subprocess.CalledProcessError:
        logging.info('Unable to checkout %s for %s', branch, path)
        return


@retry(wait_random_min=1000, wait_random_max=15000, stop_max_attempt_number=5)
def clone(repository, dest):
    """
    Call git to clone a repo.
    """
    git(['clone', repository, dest])

    target_branch = '{branch}'.format(branch=os.environ.get('GERRIT_BRANCH', 'master'))

    try:
        git([
            'rev-parse',
            '--verify',
            'origin/{branch}'.format(branch=target_branch),
        ], cwd=dest)
    except Exception:
        return

    git(['checkout', target_branch], cwd=dest)


def git(args, cwd=None):
    """
    Call git binary with provided args.
    """
    print('Executing git {cmd} in {cwd}'.format(cmd=' '.join(args), cwd=(cwd if cwd else '.')))
    return subprocess.check_call(['git'] + args, cwd=cwd)


@utils.env_stage('create', fail=True)
def checkout_code(conf, **_extra):
    """
    Clone all applicable (with 'git' attribute) projects into dest
    dir.
    """
    for arg in (conf, ):
        assert arg is not None, '{arg} must not be None'.format(arg=arg)

    # Strictly speaking not git, but we need this (local) code in staging.
    images_dir = conf.get('images_dir', 'images')
    staging_dir = conf.get('staging_dir', 'staging')
    dir_util.copy_tree(
        images_dir,
        '{staging}/{images}'.format(staging=staging_dir, images=images_dir),
        update=True,
    )

    repo_bases = conf.get('repo_bases', dict())

    for name, repo in sorted(conf.get('repos', {}).items()):
        checkout_repo(
            name,
            repo,
            repo_bases,
            staging=staging_dir,
            topic=conf.get('gerrit_topic'),
        )

    for name, props in conf.get('projects', {}).items():
        repos = props.get('git')

        # Skip projects without repo (e.g., DBs with just a schema)
        if not repos:
            continue

        # Set project sources.
        # Sorting should be enough to handle subrepos
        for repo_name in sorted(repos):
            checkout_repo(
                os.path.join(name, repo_name),
                repos[repo_name],
                repo_bases,
                staging=staging_dir,
                topic=conf.get('gerrit_topic'),
            )


def checkout_repo(name, repo, repo_bases=None, staging=None, topic=None, overwrite=False):
    """
    Clone all applicable (with 'git' attribute) projects into dest
    dir.
    """

    # Set cloning destination. Use staging dir for easier clean-up.
    clone_to = os.path.join(
        staging,
        'code',
        name,
    )

    clone_from = repo.get('git_local') or '{base}/{repo}'.format(
        repo=repo.get('repo'),
        base=repo_bases[repo['base']],
    )

    os.makedirs(os.path.dirname(clone_to), exist_ok=True)

    if os.path.exists(os.path.join(clone_to, '.git')):
        if overwrite:
            shutil.rmtree(clone_to, ignore_errors=True)
        else:
            logging.warning('%s already present, skipping', clone_to)
            return
    # Pull the code
    clone(
        clone_from,
        dest=clone_to,
    )
    # Pull specific topic or branch, if needed.
    if repo.get('commit') and repo['base'] in ('github', 'bitbucket'):
        checkout_git_branch(branch=repo['commit'], path=clone_to)
    elif topic:
        if repo['base'] == 'gerrit':
            checkout_gerrit_topic(
                topic=topic,
                path=clone_to,
                project=repo['repo'],
                repo_url=clone_from,
                gerrit_url=repo_bases['gerrit'],
            )
        elif repo['base'] in ('github', 'bitbucket'):
            checkout_git_branch(
                branch=topic,
                path=clone_to,
            )


def _main():
    repo_base = os.environ.get('DBAAS_INFRA_REPO_BASE',
                               'ssh://{user}@review.db.yandex-team.ru:9440').format(user=getpass.getuser())
    checkout_gerrit_topic(
        topic=os.environ.get('GERRIT_TOPIC'),
        path='.',
        project='mdb/dbaas-infrastructure-test',
        repo_url='{base}/mdb/dbaas-infrastructure-test.git'.format(base=repo_base),
        gerrit_url=repo_base)


if __name__ == '__main__':
    _main()
