#!/usr/bin/env python3
"""
Versions.sls state helper
"""

import argparse
import concurrent.futures
from datetime import datetime
import subprocess
import functools
from pathlib import Path

import lxml.etree
import humanfriendly
import yaml
from dateutil import parser as dt_parser
from dateutil.tz import tzutc


def get_full_uri(repo):
    """
    Get /srv uri
    """
    full_url = repo['repo_uri']
    if not full_url.endswith('/'):
        full_url += '/'
    return full_url + repo['repo_path']


def get_svn_trunk_revision(roots):
    """
    Get latest trunk change revision
    """
    root_uri = get_full_uri(roots['root'])
    proc = subprocess.Popen(['svn', 'info', '--show-item', 'last-changed-revision', root_uri], stdout=subprocess.PIPE, encoding='utf-8')
    info_output, _ = proc.communicate()
    return info_output.strip()


def format_date_diff(record_date):
    """
    Format svn date in git-like format
    """
    now = datetime.utcnow().replace(tzinfo=tzutc())
    parsed = dt_parser.parse(record_date)
    formatted = humanfriendly.format_timespan((now - parsed).total_seconds())
    return '{time} ago'.format(time=' '.join(formatted.replace(', ', ' ').split()[:2]))


def format_svn_record(record: lxml.etree.Element) -> str:
    """
    Format single svn record in human-friendly line
    """
    message = (record.findtext('msg') or '').splitlines()[0]
    return '{revision} - {message} ({date}) <{author}>'.format(
        revision=record.get('revision'),
        author=record.findtext('author'),
        date=format_date_diff(record.findtext('date')),
        message=message,
    )


def format_svn_log(log_output, revision):
    """
    Format svn log xml to more human-friendly form
    """
    tree = lxml.etree.fromstring(log_output)
    for record in reversed(tree.xpath('//logentry')):
        if record.get('revision') != str(revision):
            yield format_svn_record(record)


@functools.lru_cache(maxsize=None)
def get_svn_path_diff(uri, pinned_revision, trunk_revision):
    """
    Get formatted diff between pinned and trunk revision for path
    """
    cmd = [
        'svn',
        'log',
        uri,
        '-r{pinned}:{trunk}'.format(pinned=pinned_revision, trunk=trunk_revision),
        '--xml',
    ]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    log_output, _ = proc.communicate()
    return list(format_svn_log(log_output, pinned_revision))


def find_repo_by_path(roots, path):
    """
    Find repo (in terms of repos.sls) by versions.sls pin-path
    """
    longest_prefix = ''
    repo = {}
    for root in roots['fileroots_subrepos']:
        if path.startswith(root['mount_path']) and len(root['mount_path']) > len(longest_prefix):
            longest_prefix = root['mount_path']
            repo = root
    return repo


def show_env_diffs(roots, envconfig, svn_trunk_revision, prefix, component_name, workers):
    """
    Show paths diffs for environment
    """
    fileroots_uri = get_full_uri(roots['fileroots'])
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        path2future = {}
        for path in envconfig:
            if component_name and path != component_name:
                continue
            if prefix and not path.startswith(prefix):
                continue
            revision = envconfig[path]
            if revision == 'trunk':
                continue

            code_uri = fileroots_uri + '/' + path
            repo = find_repo_by_path(roots, path)
            if repo:
                if repo['repo_uri'].startswith('git'):
                    # don't want to support last git repo
                    # only DBM use it at that moment
                    # and git doesn't have remote API
                    continue
                code_uri = get_full_uri(repo) + '/' + path[len(repo['mount_path']):]

            path2future[path] = executor.submit(get_svn_path_diff, code_uri, revision, svn_trunk_revision)

        concurrent.futures.wait(path2future.values())

        warnings = []
        problems = get_potential_strange_components(envconfig, roots)
        latest_versions = {}
        for path in sorted(path2future):
            diffs = path2future[path].result()
            if diffs:
                print(f'    {path}')
                for line in diffs:
                    print(f'        {line}')

                version = diffs[0].split()[0]
                latest_versions[path] = version

        for path in sorted(path2future):
            diffs = path2future[path].result()
            rootpath = problems.get(path)
            if diffs and rootpath and latest_versions.get(rootpath):
                msg = "WARN: path {} may not have been changed, " + \
                      "but was simply affected by a change in subfolder {} "
                warnings.append(msg.format(rootpath, path))
        return latest_versions, warnings


def get_potential_strange_components(envconfig, roots):
    """
    return sturct like {'comp/dir/subdir': 'comp/dir', 'comp/dir/test': 'comp/dir'}
    :param envconfig:
    :param roots:
    :return:
    """
    prev_key = ''
    problems = {}
    for key in sorted(envconfig):
        if key == 'components':
            continue
        if not prev_key:
            prev_key = key
            continue
        if key.startswith(prev_key):
            found = False
            for root in roots['fileroots_subrepos']:
                if key.startswith(root['mount_path']):
                    found = True
                    break
            if not found:
                problems[key] = prev_key
        else:
            prev_key = key
    return problems


def _main():
    _pillar_dir = Path(__file__).parent

    def _default_repos():
        for fpath in [
            _pillar_dir / 'repos.sls',
            _pillar_dir / Path('../salt/components/mdb-salt-sync/conf/repos.sls'),
            Path('/opt/yandex/mdb-salt-sync/repos.sls'),
        ]:
            if fpath.exists():
                return fpath

    def _default_versions():
        for fpath in [
            _pillar_dir / 'versions.sls',
            Path('/srv/pillar/versions.sls'),
        ]:
            if fpath.exists():
                return fpath

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('-e', '--environment', type=str, default=None, help='Show only environment')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-p', '--prefix', type=str, default=None, help='Filter by prefix')
    group.add_argument('-n', '--name', type=str, default=None, help='Update only one component, search by name')
    parser.add_argument('-w', '--workers', type=int, default=16, help='Number of workers that do svn info')
    parser.add_argument('-v', '--versions', type=str, default=_default_versions(), help='versions.sls file')
    parser.add_argument('-r', '--repos', type=str, default=_default_repos(), help='repos.sls file')
    parser.add_argument('--update', action='store_true', help='update versions.sls')

    args = parser.parse_args()

    if args.update and not args.name:
        print("Failed to update: can't work without --name option")
        return

    with open(args.repos) as roots_file:
        roots = yaml.safe_load(roots_file)

    with open(args.versions) as versions_file:
        versions = yaml.safe_load(versions_file)

    trunk_revision = get_svn_trunk_revision(roots)

    if args.environment:
        print(f'{args.environment}:')
        latest_versions, warnings = show_env_diffs(roots, versions.get(args.environment, {}), trunk_revision, args.prefix, args.name, args.workers)
        if args.update:
            for path, version in latest_versions.items():
                env_versions = versions.get(args.environment, {})
                env_versions[path] = version

        if warnings:
            print()
            for warn in warnings:
                print("[{}] {}".format(args.environment, warn))
    else:
        envs_warnings = {}
        for env, envconfig in versions.items():
            print(f'{env}:')
            latest_versions, warnings = show_env_diffs(roots, envconfig, trunk_revision, args.prefix, args.name, args.workers)
            envs_warnings[env] = warnings
            for path, version in latest_versions.items():
                envconfig[path] = version

        for env, warnings in envs_warnings.items():
            if warnings:
                print()
                for warn in warnings:
                    print("[{}] {}".format(env, warn))

    if args.update:
        with open(args.versions, 'w') as versions_file:
            for key, version in versions.items():
                yaml.dump({key:version}, versions_file, indent=4, sort_keys=False)
                versions_file.write("\n")

if __name__ == '__main__':
    _main()
