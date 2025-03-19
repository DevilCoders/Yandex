#!/usr/bin/env python
import os
import argparse
import subprocess
import yaml
import errno
import shutil
import logging
import tempfile
import errno
import time

from contextlib import contextmanager


log = logging.getLogger(__name__)


@contextmanager
def atomic_open(path, mode=0o644):
    base_dir = os.path.dirname(os.path.abspath(path))
    tmp_file = tempfile.NamedTemporaryFile(mode='w+b', dir=base_dir, delete=False)
    stat_info = os.stat(base_dir)
    try:
        os.chown(tmp_file.name, stat_info.st_uid, stat_info.st_gid)
    except OSError as e:
        if e.errno != errno.EPERM:
            raise
    try:
        yield tmp_file
    finally:
        tmp_file.close()
        os.chmod(tmp_file.name, mode)
        os.rename(tmp_file.name, path)


@contextmanager
def cwd(path):
    curdir = os.getcwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(curdir)


def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


class GitRepo(object):
    def __init__(self, repo_path, bin_path='/usr/bin/git'):
        self._repo_path = os.path.abspath(repo_path)
        self._bin_path = os.path.abspath(bin_path)

    def run(self, *args):
        cmd = [self._bin_path] + list(args)
        log.info('run cmd="%s" at dir=%s', ' '.join(cmd), self._repo_path)
        with cwd(self._repo_path):
            return subprocess.check_output(cmd)

    def mirror(self, git_url):
        if not os.path.exists('{}/HEAD'.format(self._repo_path)):
            mkdir_p(self._repo_path)
            self.run('clone', '--mirror', git_url, self._repo_path)
        else:
            self.run('remote', 'update', '-p')

    def worktree(self, git_hash, path):
        self.run('worktree', 'add', path, git_hash, '--detach')


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('config')
    parser.add_argument('-l', '--log-level', default='INFO')
    return parser.parse_args()


def make_checkouts(repo, checkouts_dir, branch=None):
    mkdir_p(checkouts_dir)

    refs = 'refs/heads'
    if branch:
        refs = '{}/{}'.format(refs, branch)

    ref_map = {}
    for line in repo.run('for-each-ref', refs, "--format=%(objectname) %(refname)").splitlines():
        log.info('got line="%s"', line)
        git_hash, ref = line.strip().split()
        ref = ref.replace('refs/heads/', '')
        worktree_path = os.path.join(checkouts_dir, git_hash)
        if not os.path.exists(worktree_path):
            repo.worktree(git_hash, worktree_path)
        ref_map[ref] = worktree_path
    return ref_map


def load_config(path):
    with open(path) as f:
        return yaml.safe_load(f)


def populate_roots_config(envs_map, ref_maps, config):
    roots = {}
    for saltenv, path in envs_map.items():
        roots[saltenv] = []
        for root in config:
            root_split = root.split(':')
            if len(root_split) == 2:
                repo_name, path_in_repo = root_split
                root = path_in_repo.format(**ref_maps[repo_name])
            root = root.format(_ENV_=path)
            roots[saltenv].append(root)
    return roots


def main():
    args = get_args()
    config = load_config(args.config)
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=args.log_level.upper())
    ref_maps = {}
    log.info(config)
    git_checkouts_dir = os.path.abspath(config['git_checkouts_dir'])
    git_mirrors_dir = os.path.abspath(config['git_mirrors_dir'])
    mkdir_p(git_checkouts_dir)
    mkdir_p(git_mirrors_dir)
    for repo_name, repo_conf in config['repos'].items():
        repo_path = os.path.join(git_mirrors_dir, repo_name)
        repo = GitRepo(repo_path)
        repo.mirror(repo_conf['url'])
        ref_maps[repo_name] = make_checkouts(
            repo,
            os.path.join(git_checkouts_dir, repo_name),
            branch=repo_conf.get('branch'),
        )

    file_roots = populate_roots_config(
        envs_map=ref_maps[config['file_roots_envs_repo']],
        ref_maps=ref_maps,
        config=config['file_roots']
    )
    pillar_roots = populate_roots_config(
        envs_map=ref_maps[config['pillar_roots_envs_repo']],
        ref_maps=ref_maps,
        config=config['pillar_roots']
    )

    with atomic_open(config['file_roots_config_path']) as f:
        yaml.safe_dump(file_roots, f, default_flow_style=False)

    with atomic_open(config['pillar_roots_config_path']) as f:
        yaml.safe_dump(pillar_roots, f, default_flow_style=False)

    # cleanup
    time.sleep(config.get('cleanup_sleep', 0))
    for repo, ref_map in ref_maps.items():
        checkouts_path = os.path.join(git_checkouts_dir, repo)
        current_dirs = set(
            os.path.join(checkouts_path, d)
            for d in os.listdir(checkouts_path)
        )
        should_be_dirs = set(ref_map.values())
        for d in current_dirs - should_be_dirs:
            log.warning('remove dir=%s', d)
            shutil.rmtree(d)


if __name__ == "__main__":
    main()
