#!/usr/bin/env python3

import argparse
import errno
import fnmatch
import json
import logging
import os
import re
import shutil
import subprocess
import tarfile
import tempfile
import time
from collections import defaultdict
from contextlib import contextmanager
from pathlib import Path

import requests
import yaml
from dateutil.parser import isoparse

import sandbox.common.rest
from sandbox.common.types import resource as ctr
from sandbox.common.types import task as ctt


log = logging.getLogger()


RELEASED_ATTR_NAME = 'released'
RELEASED_TASK_STATUS = 'RELEASED'
RELEASED_AS_RE = re.compile(r'Released as ([a-z]+)')


@contextmanager
def atomic_open(path, open_mode='w+b', chmode=0o644):
    path = Path(path).resolve()
    tmp_file = tempfile.NamedTemporaryFile(mode=open_mode, dir=path.parent, delete=False)
    stat_info = os.stat(path if path.exists() else path.parent)
    try:
        os.chown(tmp_file.name, stat_info.st_uid, stat_info.st_gid)
    except OSError as e:
        if e.errno != errno.EPERM:
            raise
    try:
        yield tmp_file
    finally:
        tmp_file.close()
        os.chmod(tmp_file.name, chmode)
        os.rename(tmp_file.name, path)


@contextmanager
def atomic_dir(path, mode=0o755):
    path = Path(path).resolve()
    tmp = tempfile.mkdtemp(prefix=f'{path}.TMP.')
    stat_info = os.stat(path if path.exists() else path.parent)
    os.chown(tmp, stat_info.st_uid, stat_info.st_gid)
    try:
        yield Path(tmp)
        os.chmod(tmp, mode)
        os.rename(tmp, path)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def parse_commit_time(date_str):
    return isoparse(date_str)


# TODO: use bare repo or api? Bare repo can't do arc log for path
class ArcRepo:
    def __init__(self, arc_token_path, base_path, bin_path, base_url):
        self._arc_token_path = arc_token_path
        self.base_url = base_url
        self._bin_path = shutil.which(bin_path)
        if not self._bin_path:
            raise RuntimeError(f'{bin_path = } not found')
        self.path = Path(base_path).resolve()
        self.mount_path = Path(self.path, 'arcadia').resolve()
        self.store_path = Path(self.path, 'store').resolve()
        self._mounted = None

    @property
    def token(self):
        return self.get_arc_token(self._arc_token_path)

    @staticmethod
    def get_arc_token(path):
        path = Path(path).expanduser().resolve()
        with open(Path(path).resolve()) as f:
            return f.read().strip()

    def mount_arc(self):
        log.info("mount arc to %r", self.path)
        self.path.mkdir(parents=True, exist_ok=True)
        return self._call_arc(
            'mount', '-m', str(self.mount_path), '-S', str(self.store_path), '--allow-other',
            cwd=self.path,
            ensure_mounted=False,
        )

    def _is_arc_mounted(self):
        log.info('Looking for arc at %s', self.mount_path)
        with open("/proc/mounts") as pm:
            for line in pm.readlines():
                fs_type, mount_path, *_ = line.split()
                if fs_type == "arc":
                    mount_path = Path(mount_path).resolve()
                    log.info('Found arc at %s', mount_path)
                    if self.mount_path == mount_path:
                        return True
        return False

    def ensure_mounted(self):
        if not self._mounted:
            if not self._is_arc_mounted():
                self.mount_arc()
            self._mounted = True
        return self._mounted

    def _call_arc(self, *args, cwd=None, ensure_mounted=True):
        if ensure_mounted:
            self.ensure_mounted()
        cwd = cwd or self.mount_path
        cmd = [str(self._bin_path)]
        cmd.extend(args)
        log.debug('Calling arc util: "%s" at cwd="%s"', subprocess.list2cmdline(cmd), cwd)
        ret = subprocess.check_output(cmd, cwd=cwd)
        return ret.decode('utf-8').strip()

    def gc(self):
        self._call_arc('gc')

    def export(self, commit, src, dst):
        src = src.lstrip('/')  # relative to arc mount path
        self._call_arc('export', commit, str(src), '--to', str(dst))

    def is_path_present_in_ref(self, ref, path):
        try:
            self._call_arc('rev-parse', f'{ref}:{path}')
            return True
        except Exception as e:
            log.warning(vars(e))
            return False

    def fetch(self, branch):
        self._call_arc('fetch', branch, '--verbose')
        return f'arcadia/{branch}'

    def branch_last_commit(self, branch):
        remote_name = self.fetch(branch)
        return self._call_arc('rev-parse', remote_name)

    def get_latest_commit_for_paths(self, branch, paths):
        last_branch_commit = self.branch_last_commit(branch)
        latest_commit = None
        for path in paths:
            # If path not present in branch and path does not end with / then arc will try to fetch path as remote branch.
            # avoid it!
            path = str(path).rstrip('/') + '/'
            # FIXME: does not detect deletions
            commits = json.loads(self._call_arc('log', '-n1', '--json', last_branch_commit, path))
            if commits:
                commit = commits[0]
                # TODO: better way than time comparision?
                if latest_commit is None or parse_commit_time(commit['date']) > parse_commit_time(
                    latest_commit['date']
                ):
                    latest_commit = commit
            else:
                log.warning('No commits found for path %s in %s %s', path, branch, last_branch_commit)

        if not latest_commit:
            # raise RuntimeError(f'there are no commits for any path in {remote_name}')
            log.info('There are no commits for paths %s in %s %s', paths, branch, last_branch_commit)
            return None

        return latest_commit['commit']

    def _call_arc_api(self, path, **kwargs):
        headers = {'Authorization': f'OAuth {self.token}'}
        url = '{}{}'.format(self.base_url, path)
        res = requests.get(url=url, headers=headers, **kwargs)

        log.debug('Arc api response text: %s', res.text)
        res.raise_for_status()

        return res.json()['data']

    def get_members(self, group):
        # Groups config can be found on arcadia/trunk/groups/
        res = self._call_arc_api(f'/v2/groups/{group}/members')
        log.debug('group %s members: %s', group, res)
        return [u['name'] for u in res]

    def get_branches_by_members(self, members):
        out = set()

        for m in members:
            url = f'/v2/repos/arc/branches?name=users/{m}/&fields=name&limit=100'  # FIXME: hardcoded limit
            for branch in self._call_arc_api(url):
                out.add(branch['name'])

        log.debug('Found branches: %s', out)
        return out

    def get_reviews(self, path, query=None):
        query = query or {'open': []}
        _query = {}
        _query.update(query)
        _query.update({'path': [f'/{path.lstrip("/")}']})
        # open();status(published);something(one,two,three)
        query_str = ';'.join(
            f'{k}({",".join(_ for _ in v)})'
            for k, v in _query.items()
        )
        log.debug('review query: %s', query_str)
        params = {
            'query': query_str,
            'fields': 'review_requests(id,issues,active_diff_set(arc_branch_heads(from_id,to_id,merge_id)),vcs(type,name,branch,from_branch,to_branch))',
        }
        data = self._call_arc_api('/v1/review-requests', params=params)
        return data['review_requests']


def write_salt_config(config_path, envs, roots_config):
    out = {}
    for env_name, env_checkout_path in envs.items():
        out[env_name] = []
        for item in roots_config:
            out[env_name].append(item.format(_ENV_CHECKOUT_PATH_=env_checkout_path))

    config_path = Path(config_path).resolve()
    config_path.parent.mkdir(parents=True, exist_ok=True)
    with atomic_open(config_path, 'w') as f:
        yaml.safe_dump(out, f, default_flow_style=False)


def read_config(path):
    with open(path, "r") as cf:
        return yaml.safe_load(cf)


def ensure_arc_checkouts(repo: ArcRepo, base_checkouts_path, commit_id, paths):
    base_checkouts_path = Path(base_checkouts_path).resolve()
    base_checkouts_path.mkdir(parents=True, exist_ok=True)
    commit_dir = Path(base_checkouts_path, f'arc-{commit_id}').resolve()
    if not commit_dir.is_dir():
        with atomic_dir(commit_dir) as temp_dir:
            for path in paths:
                if repo.is_path_present_in_ref(commit_id, path):
                    log.info(f'Export {commit_id = } {path = } to {commit_dir = }')
                    repo.export(commit_id, path, temp_dir)
                else:
                    log.warning(f'{path = } not present in {commit_id = }. skip export')
    return commit_dir


def arc_reviews(repo: ArcRepo, base_checkouts_path, paths, query=None):
    envs = {}
    reviews = []
    for path in paths:
        reviews.extend(repo.get_reviews(path, query))

    for review in reviews:
        commit_id = review['active_diff_set']['arc_branch_heads']['from_id']  # Not sure
        review_id = review['id']
        try:
            branch = review['vcs']['from_branch']
        except KeyError:
            log.info(f'Skip review {review_id = } without branch')
            continue
        log.info(f'Found {commit_id = } {branch = } {review_id = }')
        commit_dir = str(ensure_arc_checkouts(repo, base_checkouts_path, commit_id, paths))
        envs[commit_id] = commit_dir
        envs[branch] = commit_dir
        envs[f'pr-{review_id}'] = commit_dir
    return envs


def arc_branches(repo: ArcRepo, branches, base_checkouts_path, paths, always_use_head=False):
    envs = {}
    for branch in branches:
        if always_use_head:
            commit_id = repo.branch_last_commit(branch)
        else:
            commit_id = repo.get_latest_commit_for_paths(branch, paths)
        if commit_id is not None:
            log.info(f'Found {commit_id = } {branch = }')
            commit_dir = str(ensure_arc_checkouts(repo, base_checkouts_path, commit_id, paths))
            envs[commit_id] = commit_dir
            envs[branch] = commit_dir
    return envs


def arc_branches_from_users(
    repo: ArcRepo, base_checkouts_path, paths, group=None, users=None, always_use_head=False, branch_glob='*'
):
    members = set()
    if group:
        members.update(repo.get_members(group))
    if users:
        members.update(users)
    if not members:
        raise RuntimeError(f'Failed to find any users for {group = } and {users = }')
    branches = repo.get_branches_by_members(members)
    branches = [b for b in branches if fnmatch.fnmatch(b, branch_glob)]
    return arc_branches(repo, branches, base_checkouts_path, paths, always_use_head)


arc_static_branches = arc_branches


# almost copy paste. dumb way to get releases
def by_last_released_task(resource_type, attrs=None, stages=[ctt.ReleaseStatus.STABLE], last_n=100):
    # type: (Any, Optional[dict], ctt.ReleaseStatus, int) -> Tuple[Optional[int], str]
    """
    Get resource of specified resource type, released to specified stage most lately with its release time.
    Get last_n resources of specified resource type, released to all stages, sorted by descending id.
    Get tasks audit for these resources.
    Filter released to specific stage times
    Choose resource with max released task time and return it (as int) with time (as human readable str).
    """
    result = {}
    sb_rest_client = sandbox.common.rest.Client()
    resources = sb_rest_client.resource.read(
        type=resource_type,
        status=ctr.State.READY,
        attr_name=RELEASED_ATTR_NAME,
        attrs=attrs,
        limit=last_n,
    )['items']

    log.debug('Got resource candidates: %s', [(i['id'], i['attributes'][RELEASED_ATTR_NAME]) for i in resources])
    if not resources:
        log.warning('No released resource for %s with attrs %s', resource_type, attrs)
        return result

    # assume, that there are no same resources in one task
    task_resource_index = {i['task']['id']: i for i in resources}
    audit = sb_rest_client.task.audit.read(id=list(task_resource_index.keys()))
    released_to = defaultdict(list)

    for audit_item in audit:
        if audit_item.get('status') == RELEASED_TASK_STATUS:
            stage_match = RELEASED_AS_RE.match(audit_item.get('message', ''))
            if stage_match:
                released_to[stage_match.group(1)].append((audit_item['task_id'], audit_item['time']))

    log.debug('Got released statuses from audit: %s', released_to)
    cancelled_tasks = {
        task_id: time for task_id, time in sorted(released_to[str(ctt.ReleaseStatus.CANCELLED)], key=lambda x: x[1])
    }  # sort by time, because we need to store last cancelled release time for task

    for stage in stages:
        for task_id, time in reversed(sorted(released_to[str(stage)], key=lambda x: x[1])):
            if task_id not in cancelled_tasks or time > cancelled_tasks[task_id]:
                result[stage] = {
                    'resource': task_resource_index[task_id],
                    'time': time,
                }
                break

    log.debug('Resource by last released task = %s', result)
    return result


# TODO: sky/mds
def sandbox_download(resource, dst):
    url = resource['http']['proxy']
    r = requests.get(url, stream=True)
    if r.status_code != 200:
        raise RuntimeError(f'Failed to download {url}: {r}')
    tar = tarfile.open(fileobj=r.raw, mode='r:gz')
    tar.extractall(dst)
    tar.close()


def ensure_sandbox_checkout(base_checkouts_path, resource):
    resource_id = resource['id']
    base_checkouts_path = Path(base_checkouts_path).resolve()
    base_checkouts_path.mkdir(parents=True, exist_ok=True)
    resource_dir = Path(base_checkouts_path, f'sandbox-{resource_id}').resolve()
    if not resource_dir.is_dir():
        log.info(f'Download {resource_id = } to {resource_dir = }')
        with atomic_dir(resource_dir) as temp_dir:
            sandbox_download(resource, temp_dir)
    return resource_dir


def sandbox_releases(resource_type, releases, base_checkouts_path, attrs=None):
    envs = {}

    releases_to_resources = by_last_released_task(
        resource_type,
        stages=releases,
        attrs=attrs,
    )
    for release_status, result in releases_to_resources.items():
        resource = result['resource']
        res_id = resource['id']
        log.info(f'Found {res_id = } {release_status = }')
        resource_dir = str(ensure_sandbox_checkout(base_checkouts_path, resource))
        for env_name in releases[release_status]:
            envs[env_name] = resource_dir
            envs[f'sandbox-{res_id}'] = resource_dir

    return envs


def cleanup(envs, sources):
    base_paths = set(s['options']['base_checkouts_path'] for s in sources)
    paths_in_use = set(envs.values())
    for base_path in base_paths:
        for checkout_dir in os.listdir(base_path):
            path = Path(base_path, checkout_dir).resolve()
            if str(path) not in paths_in_use:
                log.warning('Removing %s', path)
                shutil.rmtree(path)


def build_envs(config, repo):
    envs = {}
    for env_source in config['env_sources']:
        t = env_source['type']
        log.debug('processing env source %s', t)
        if t == 'arc_reviews':
            envs.update(arc_reviews(repo, **env_source['options']))
        elif t == 'arc_branches_from_users':
            envs.update(arc_branches_from_users(repo, **env_source['options']))
        elif t == 'arc_static_branches':
            envs.update(arc_branches(repo, **env_source['options']))
        elif t == 'sandbox_releases':
            envs.update(sandbox_releases(**env_source['options']))
        else:
            raise RuntimeError(f'unkown env source type "{t}" in config')

    return envs


def write_envs(config, envs):
    log.debug('writing envs: %s', envs)
    for t in ('pillar', 'file'):
        write_salt_config(config_path=config[f'{t}_roots_config_path'], envs=envs, roots_config=config[f'{t}_roots'])


def write_monrun_state(path, status, message):
    with atomic_open(path) as f:
        f.write(f'{status};{message}'.replace('\n', ' | ').encode('utf-8'))


def check(state_path: Path, lag_threshold):
    state_path = state_path.resolve()
    if not state_path.is_file():
        return 2, f'{state_path} not found'

    lag = int(time.time() - state_path.stat().st_mtime)
    if lag > lag_threshold:
        return 2, f'salt-arc state file too old: lag is {lag}'

    with open(state_path) as f:
        status, message = f.read().strip().split(';', 1)
        message = f'{message} {lag} seconds ago'
        return int(status), message


def parse_args():
    parser = argparse.ArgumentParser("Generate saltenvs from arcadia branches")
    parser.add_argument('-c', '--config', type=Path, default=Path('/etc/yandex/salt/arc2salt.yaml'))
    parser.add_argument('--arc-token-path', type=Path, default=('~/.arc/token'))
    parser.add_argument('--arc-bin', type=Path, default=shutil.which('arc'))
    parser.add_argument('--arcanum-api-url', type=str, default='https://a.yandex-team.ru/api')
    parser.add_argument("--debug", action="store_true", help="Debug mode")
    parser.add_argument('--log-level', type=str, default='INFO')
    parser.add_argument("--log-file", type=Path, help="Log to file")
    parser.add_argument('--cleanup', action='store_true', help='Remove unused checkouts')
    parser.add_argument('--arc-gc', action='store_true')
    parser.add_argument('--no-write', action='store_true', help='Do not write envs to configs')
    parser.add_argument('--print-envs', action='store_true', help='Print envs to stdout')
    parser.add_argument('--monrun-state-file', type=Path, default=Path('/var/tmp/salt-arc-monrun.state'))
    parser.add_argument('--check', action='store_true', help='Print monrun check data end exit')
    parser.add_argument('--lag-threshold', type=int, default=180, help='crit if state file changed more then lag-threshold seconds ago')

    return parser.parse_args()


def main():
    args = parse_args()

    if args.check:
        try:
            status, message = check(args.monrun_state_file, args.lag_threshold)
        except Exception as e:
            status, message = 2, str(e)

        print(f'{status};{message}'.replace('\n', ' | '))
        exit(status)

    try:
        logging.basicConfig(
            level='DEBUG' if args.debug else args.log_level.upper(),
            format="%(asctime)s %(levelname)s %(message)s",
            filename=args.log_file,
        )

        config = read_config(args.config)
        repo = ArcRepo(
            arc_token_path=args.arc_token_path,
            base_path=config['arc-path'],
            bin_path=args.arc_bin,
            base_url=args.arcanum_api_url,
        )

        envs = build_envs(config, repo)
        if not args.no_write:
            write_envs(config, envs)

        if args.print_envs:
            print(json.dumps(envs, indent=4))

        if args.cleanup:
            cleanup(envs, config['env_sources'])

        if args.arc_gc:
            repo.gc()

    except Exception as e:
        log.exception('Something broken')
        status = 2
        message = f'something broken: {e}'
        raise
    else:
        status = 0
        message = f'updated {len(envs)} envs'
    finally:
        if args.monrun_state_file:
            write_monrun_state(args.monrun_state_file, status, message)


if __name__ == "__main__":
    main()
