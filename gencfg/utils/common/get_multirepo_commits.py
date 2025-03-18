#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from config import REPO_DATA, MAIN_DIR
from core.gitapi import GitRepository


def get_parser():
    parser = ArgumentParserExt(usage='usage: %(prog)s [options]', prog="PROG")

    parser.add_argument("-s", "--startts", type=argparse_types.xtimestamp, default=None,
                        help="Optional. Start time")
    parser.add_argument("--startcommits", type=argparse_types.jsondict, default=None,
                        help="Optional. Start commits as dict like { \"balancer\" : \"f4089d\", \"db\" : \"6e3fb5\", \"main\" : \"6cd1a0\" }")
    parser.add_argument("-e", "--endts", type=argparse_types.xtimestamp, default=None,
                        help="Optinal. End time")
    parser.add_argument("--endcommits", type=argparse_types.jsondict, default=None,
                        help="Optional. End commits as dict like { \"balancer\" : \"f4089d\", \"db\" : \"6e3fb5\", \"main\" : \"6cd1a0\" }")
    parser.add_argument("-l", "--log-local", dest="log_remote", action="store_false", default=True,
                        help="Optional. Get tuples of local commits rather than origin/master commits")

    return parser


def normalize(options):
    if options.startts is None and options.endts is None and options.startcommits is None and options.endcommits is None:
        raise Exception("You must specify <--startts> or <--endts> option")
    if (options.startts is not None or options.endts is not None) and (
            options.startcommits is not None or options.endcommits is not None):
        raise Exception("You can not mix <--startts --endts> options with <--startcommits --endcommits> ones")

    if options.startts is not None and options.startcommits is not None:
        raise Exception("Options --startts and --startcommits are mutually exclusive")
    if options.endts is not None and options.endcommits is not None:
        raise Exception("Options --endts and --endcommits are mutually exclusive")

    if options.startcommits is not None and set(REPO_DATA.keys()) != set(options.startcommits.keys()):
        raise Exception("Dict --startcommits must contain exactly <%s> keys" % (",".join(REPO_DATA.keys())))
    if options.endcommits is not None and set(REPO_DATA.keys()) != set(options.endcommits.keys()):
        raise Exception("Dict --endcommits must contain exactly <%s> keys" % (",".join(REPO_DATA.keys())))


def main(options):
    current_multicommit = dict()
    all_commits = []

    for repo_name in REPO_DATA:
        git_repo = GitRepository(os.path.join(MAIN_DIR, REPO_DATA[repo_name]['localpath']))

        if options.startts is not None or options.endts is not None:
            repo_commits = git_repo.get_commits_date_range(startts=options.startts, endts=options.endts)
        else:
            startcommit = options.startcommits[repo_name] if options.startcommits else None
            endcommit = options.endcommits[repo_name] if options.endcommits else None
            repo_commits = git_repo.get_commits_commit_range(startcommit=startcommit, endcommit=endcommit)

        all_commits.extend(map(lambda x: (repo_name, x), repo_commits))
        if len(repo_commits) > 0:
            current_multicommit[repo_name] = git_repo.get_previous_commit(repo_commits[-1].commit)
        else:
            current_multicommit[repo_name] = git_repo.get_last_commit()

    all_commits.sort(cmp=lambda (x1, y1), (x2, y2): cmp((y1.date, x1), (y2.date, x2)))

    result = []
    for repo_name, commit in all_commits:
        current_multicommit[repo_name] = commit
        result.append((copy.deepcopy(current_multicommit), repo_name))

    return result


def print_result(result):
    for commit_dict, last_changed_repo in result:
        s = []
        for k in sorted(commit_dict.keys()):
            s.append('"%s" : "%s"' % (k, commit_dict[k].commit))
        author = commit_dict[last_changed_repo].author
        print "Commits: { %s } Changed: %s Author: %s" % (", ".join(s), last_changed_repo, author)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    result = main(options)
    print_result(result)
