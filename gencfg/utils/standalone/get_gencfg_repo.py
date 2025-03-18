#!/skynet/python/bin/python

import os
import sys
import shutil
import subprocess
from argparse import ArgumentParser
import tempfile
import xmlrpclib
import time
import socket
import requests

# REPO_MAPPING = {
#     "full": "",
#     "db": "/db",
#     "balancer": "/custom_generators/balancer_gencfg",
# }

class ERepoType(object):
    FULL = 'full'  # full gencfg (src+db)
    DB = 'db'  # gencfg db
    BALANCER = 'balancer'  # gencfg balancer
    ALL = (FULL, DB, BALANCER)


GENCFG_TRUNK_DATA_PATH = "svn+ssh://arcadia.yandex.ru/arc/trunk/data/gencfg_db"
GENCFG_TRUNK_SRC_PATH = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/gencfg"

GENCFG_BRANCH_PREFIX = "svn+ssh://arcadia.yandex.ru/arc/branches/gencfg"
GENCFG_TAG_PREFIX = "svn+ssh://arcadia.yandex.ru/arc/tags/gencfg"

GENCFG_BALANCER_POSTFIX = "custom_generators/balancer_gencfg"


class DbDirHolder(object):
    def __init__(self, path, temporary=False):
        self.path = path
        self.temporary = temporary

    def __del__(self):
        if self.temporary:
            shutil.rmtree(self.path)


# COPYPASTE from gaux.aux_utils to make this code independant
def run_command(args, raise_failed=True, cwd=None, close_fds=False, stdin=None):
    try:
        if stdin is None:
            p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd, close_fds=close_fds)
        else:
            p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd,
                                 close_fds=close_fds)
    except Exception, e:
        if raise_failed:
            raise Exception("subpocess.Popen for <<%s>> failed: %s" % (args, e))
        else:
            return 1, 'Got unknown error %s' % e, ''

    if not stdin is None:
        p.stdin.write(stdin)  # Is it correct ?
        p.stdin.close()

    out, err = p.communicate()
    if p.returncode != 0 and raise_failed:
        raise Exception("Command <<%s>> returned %d\nStdout:%s\nStderr:%s" % (args, p.returncode, out, err))

    return p.returncode, out, err


def run_command_with_retry(attempts, command, clear_command=None, **kwargs):
    timeout_table = [1, 5, 10, 20]
    for attempt in xrange(attempts):
        try:
            return run_command(command, **kwargs)
        except Exception:
            if clear_command:
                run_command(clear_command, **kwargs)
            if attempt == attempts - 1:
                raise

        time.sleep(timeout_table[min(attempt, len(timeout_table) - 1)])


def parse_cmd():
    parser = ArgumentParser(description="Downloads resource (file) from sandbox via skynet")
    parser.add_argument("-t", "--type", type=str, dest="repo_type", required=True,
                        choices=ERepoType.ALL,
                        help="Obligatory. What we want to checkout: {}".format(ERepoType.ALL))
    parser.add_argument("-d", "--dest", type=str, dest="dest", default=None,
                        help="Optional. Destination path. If unspecified, clone to random temporary dir")
    parser.add_argument("-r", "--revision", type=int, default=None,
                        help="Optional. Specific revision update to")
    parser.add_argument("--tag", type=str, default=None,
                        help="Optional. Checkout to specified tag (incompatable with revision)")
    parser.add_argument("--temporary", action="store_true", default=False,
                        help="Optional. Checkouted repo is temporary (will be removed on program exit)")
    parser.add_argument("--load-db-cache", action = "store_true", default = False,
                        help="Optional. Load db cache to checkouted repository")
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose",
                        default=False,
                        help="Obligatory. Destination path")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def api_main(dstdir=None, repo_type='db', revision=None, tag=None, temporary=False, verbose=False, load_db_cache=False):
    options_dict = {
        'repo_type': repo_type,
        'dest': dstdir,
        'revision': revision,
        'tag': tag,
        'temporary': temporary,
        'verbose': verbose,
        'load_db_cache' : load_db_cache,
    }

    options = type("ApiMainOption", (), options_dict)

    return main(options)


def __clone_repo(url, dest_dir, revision=None):
    """Clone repo to specified directory"""

    # first clone trunk
    run_command_with_retry(3, command=['svn', 'checkout', url, dest_dir, '--non-interactive'],
                           clear_command=['svn', 'cleanup', dest_dir, '--non-interactive'])
    run_command_with_retry(3, command=['svn', 'update', '--non-interactive'],
                           clear_command=['svn', 'cleanup', dest_dir, '--non-interactive'], cwd=dest_dir)

    # switch to revision/tag
    if revision is not None:
        run_command_with_retry(3, command=['svn', 'update', '-r', str(revision), '--non-interactive'],
                               clear_command=['svn', 'cleanup', dest_dir, '--non-interactive'], cwd=dest_dir)


def clone(dest, repo_type, revision=None, tag=None):
    if tag in (None, 'trunk'):  # checking out trunk
        if repo_type == ERepoType.FULL:
            __clone_repo(GENCFG_TRUNK_SRC_PATH, dest, revision=revision)
            __clone_repo(GENCFG_TRUNK_DATA_PATH, os.path.join(dest, 'db'), revision=revision)
        elif repo_type == ERepoType.DB:
            __clone_repo(GENCFG_TRUNK_DATA_PATH, dest, revision=revision)
        elif repo_type == ERepoType.BALANCER:
            url = '{}/{}'.format(GENCFG_TRUNK_SRC_PATH, GENCFG_BALANCER_POSTFIX)
            __clone_repo(url, dest, revision=revision)
        else:
            raise Exception('Unknown repo_type <{}>'.format(repo_type))
    else:  # checking out tag
        if repo_type == ERepoType.FULL:
            url = '{}/{}'.format(GENCFG_TAG_PREFIX, tag)
            __clone_repo(url, dest, revision=revision)
        elif repo_type == ERepoType.DB:
            url = '{}/{}/db'.format(GENCFG_TAG_PREFIX, tag)
            __clone_repo(url, dest, revision=revision)
        elif repo_type == ERepoType.BALANCER:
            url = '{}/{}/{}'.format(GENCFG_TAG_PREFIX, tag, GENCFG_BALANCER_POSTFIX)
            __clone_repo(url, dest, revision=revision)
        else:
            raise Exception('Unknown repo_type <{}>'.format(repo_type))


def main(options):
    if options.revision is not None and options.tag is not None:
        raise Exception("Options --revision and --tag are mutually exclusive")

        # make dest dir
    #    if options.dest is not None and options.temporary and os.path.exists(options.dest):
    #        raise Exception("Can not use option --temporary on existing dir <%s>" % options.dest)
    #    if options.dest is None and not options.temporary:
    #        raise Exception("You must add --temporary option when not specifiing --dest")
    if options.dest is None:
        temp_dir = os.environ.get('TMPDIR', '/var/tmp')
        options.dest = tempfile.mkdtemp(prefix="gencfg-%s-" % options.repo_type, dir=temp_dir)


    # checkout to dest dir
    clone(options.dest, options.repo_type, revision=options.revision, tag=options.tag)

    # load db cache
    if options.load_db_cache:
        # set directory to download to
        if options.repo_type == "balancer":
            raise Exception("Can not load db cache when cloning <balancer> repo")
        elif options.repo_type == "db":
            cache_dir = os.path.join(options.dest, "cache")
        elif options.repo_type == "full":
            cache_dir = os.path.join(options.dest, "db", "cache")

        url = 'https://sandbox.yandex-team.ru:443/api/v1.0/resource?limit=1&type=GENCFG_DB_CACHE&state=READY'

        try:
            requests.packages.urllib3.disable_warnings()
        except Exception:
            pass

        resources_as_json = requests.get(url).json()

        skynet_id = resources_as_json['items'][0]["skynet_id"]
        skynet_resource_name = resources_as_json['items'][0]["file_name"]

        # download resource
        import api.copier
        copier = api.copier.Copier()
        tmp_dir = tempfile.mkdtemp()
        os.chmod(tmp_dir, 0o777)
        try:
            copier.get(skynet_id, tmp_dir, network=api.copier.Network.Backbone).wait()
            shutil.move(os.path.join(tmp_dir, skynet_resource_name), cache_dir)
        except Exception:  # do not treat cache download errors as real errors
            pass

    result = DbDirHolder(options.dest, options.temporary)

    if options.verbose:
        print 'Checked out repo <%s> at <%s>' % (options.repo_type, options.dest)

    return result


if __name__ == '__main__':
    options = parse_cmd()
    result = main(options)
