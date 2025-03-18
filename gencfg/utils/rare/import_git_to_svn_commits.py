#!/skynet/python/bin/python

"""
    Import git commits to svn repository
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.gitapi import GitRepository
from gaux.aux_utils import run_command


class EFileStatus(object):
    ADDED = 'A'
    MODIFIED = 'M'
    REMOVED = 'R'
    ALL = [ADDED, MODIFIED, REMOVED]


PATCH_FILE = "/var/tmp/migrate.patch"


class GitCommit(object):
    """
        Object, representing git commit"
    """

    def __init__(self, gitpath, commit):
        self.gitpath = gitpath
        self.commit = commit

        # fill modified files
        self.modified_files = []
        out = run_command(["git", "diff-tree", "-r", commit.commit], cwd=gitpath)[1]
        for line in out.strip().split('\n')[1:]:
            prefix, _, filename = line.partition('\t')
            status = prefix.rpartition(' ')[2]
            if status not in EFileStatus.ALL:
                raise Exception("Commit <%s> has unknown status <%s> for file <%s>" % (commit.commit, status, filename))
            self.modified_files.append((filename, status))

        # get diff as patch
        self.patch = run_command(["git", "show", commit.commit], cwd=gitpath)[1]

        print "Loaded commit <%s>" % commit
        print self.as_str()

    def add_commit_to_svn(self, svnpath):
        print "Adding commit <%s> to svn repo at path <%s>" % (self.commit.commit, svnpath)

        with open(PATCH_FILE, 'w') as f:
            f.write(self.patch)
        print "    Added patch to file <%s>" % PATCH_FILE

        run_command(["patch", "-p1", "-i", PATCH_FILE], cwd=svnpath)
        print "    Applied patch to svn repo at <%s>" % svnpath

        to_add = filter(lambda (fname, status): status == EFileStatus.ADDED, self.modified_files)
        to_add = map(lambda (fname, status): fname, to_add)
        if len(to_add) > 0:
            run_command(["svn", "add"] + to_add, cwd=svnpath)
            print "    Added new files %s" % (" ".join(to_add))

        to_remove = filter(lambda (fname, status): status == EFileStatus.REMOVED, self.modified_files)
        to_remove = map(lambda (fname, status): fname, to_remove)
        if len(to_remove) > 0:
            run_command(["svn", "rm"] + to_remove, cwd=svnpath)
            print "    Removed new files %s" % (" ".join(to_remove))

        to_modify = filter(lambda (fname, status): status == EFileStatus.MODIFIED, self.modified_files)
        to_modify = map(lambda (fname, status): fname, to_modify)
        if len(to_modify) > 0:
            print "    Modified files (not needed to do anything): %s" % (" ".join(to_modify))

    def as_str(self):
        result = ["Commit <%s>:" % self.commit.commit,
                  "    Author: %s" % self.commit.author,
                  "    Message: %s" % self.commit.message,
                  "    Date: %s" % time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(self.commit.date)),
                  "    Modified files:"]
        for (fname, status) in self.modified_files:
            result.append("        File %s: %s" % (fname, status))

        return "\n".join(result)


def get_parser():
    parser = ArgumentParserExt(description="Import commits from git to svn")
    parser.add_argument("--git-path", type=str, required=True,
                        help="Obligatory. Path to git repository dir (to import from)")
    parser.add_argument("--svn-path", type=str, required=True,
                        help="Obligatory. Path to svn repository dir (to import to)")
    parser.add_argument("--log-path", type=str, required=True,
                        help="Obligatory. Path to log dir")
    parser.add_argument("--first-commit", type=str, required=True,
                        help="Obligatory. First commit to process")
    parser.add_argument("--last-commit", type=str, default=None,
                        help="Optional. Last commit to process")

    return parser


def main(options):
    # get list of commits
    repo = GitRepository(options.git_path)
    commits = list(
        reversed(repo.get_commits_commit_range(startcommit=options.first_commit + "^", endcommit=options.last_commit)))
    extended_commits = map(lambda x: GitCommit(options.git_path, x), commits)

    # apply commits one by one
    for commit in extended_commits:
        commit.add_commit_to_svn(options.svn_path)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
