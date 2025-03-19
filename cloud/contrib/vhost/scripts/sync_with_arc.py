#!/usr/bin/python3

import subprocess
import json
import argparse
import os
import shutil
import tarfile


ARC_MAIN_BRANCH = "trunk"
GIT_MAIN_BRANCH = "master"

ARC_SYNC_PREFIX = "bb-sync-"
ARC_VHOST_PATH = "cloud/contrib/vhost"


class VCS(object):
    def __init__(self, repo_path):
        self.repo_path = repo_path


    def _fix_str(self, input):
        # we don't use universal_newlines in subprocess.* as we don't want
        # input garbled; do explicit conversions instead
        if type(input) == str:
            return input.encode()
        return input


    def test(self, *args, input=None):
        cmd_args = [self.cmd] + list(args)
        ret = subprocess.run(cmd_args, cwd=self.repo_path,
                             stdout=subprocess.DEVNULL,
                             stderr=subprocess.DEVNULL,
                             input=self._fix_str(input))
        return ret.returncode == 0


    def call(self, *args, input=None):
        cmd_args = [self.cmd] + list(args)
        subprocess.run(cmd_args, cwd=self.repo_path, check=True,
                       input=self._fix_str(input))


    def output(self, *args, input=None):
        cmd_args = [self.cmd] + list(args)
        ret = subprocess.check_output(cmd_args, cwd=self.repo_path,
                                      input=self._fix_str(input))
        return ret.decode()


    def popen(self, *args, **kwargs):
        cmd_args = [self.cmd] + list(args)
        return subprocess.Popen(cmd_args, cwd=self.repo_path, **kwargs)


class Git(VCS):
    cmd = 'git'

    def rev_parse(self, arg, abbrev_ref=True):
        args = ["rev-parse", arg]
        if abbrev_ref:
            args.insert(1, "--abbrev-ref")

        return self.output(*args).strip()


    def get_current_branch(self):
        return self.rev_parse("HEAD")


    def get_log(self, base, top='HEAD'):
        out = self.output("log", "--pretty=%H,%s", "--reverse",
                          "{}..{}".format(base, top))

        return [c.split(",", 1) for c in out.splitlines()]


    def get_commit_body(self, sha):
        return self.output("show", "--pretty=%B", "--quiet", sha)


    def get_parent(self, sha):
        return self.rev_parse("{}^".format(sha), False)


    def extract_commit(self, sha, target_dir):
        shutil.rmtree(target_dir)
        os.mkdir(target_dir)

        proc = self.popen("archive", "--format=tar", sha,
                          stdout=subprocess.PIPE)

        with tarfile.open(fileobj=proc.stdout, mode="r|*") as tar:
            tar.extractall(target_dir)

        ret = proc.wait()
        if ret != 0:
            raise subprocess.CalledProcessError(ret, "git archive")


    def hash_tree(self, ls_tree):
        tree = []
        subtree = []
        cur_dir = ''


        for mode, type, oid, name in ls_tree:
            assert type == 'blob'
            i = name.find('/')
            d, n = ('', name) if i == -1 else (name[:i], name[i + 1:])

            if d != cur_dir:
                if cur_dir:
                    tree.append((0o40000, 'tree', self.hash_tree(subtree),
                                cur_dir))
                    subtree = []
                cur_dir = d
            if d:
                subtree.append((mode, type, oid, n))
            else:
                tree.append((mode, type, oid, n))

        if cur_dir:
            tree.append((0o40000, 'tree', self.hash_tree(subtree), cur_dir))

        input = '\n'.join('{:o} {} {}\t{}'.format(*row) for row in tree)
        return self.output('mktree', input=input).strip()


    def latest_commit_by_tree(self, tree, head='HEAD', maxdepth=1000):
        out = self.output('log', '--pretty=%H %T', '-n', str(maxdepth), head)
        for line in out.splitlines():
            commit, commit_tree = line.split(' ')
            if commit_tree == tree:
                return commit

        raise ValueError('no commit with tree {}'.format(tree))


class Arc(VCS):
    cmd = 'arc'


    def get_branches(self):
        out = self.output("branch", "--all", "--list", "--json")
        return json.loads(out)


    def get_current_branch(self):
        for branch in self.get_branches():
            if branch.get('current', False):
                return branch['name']
        return None


    def add_changes_and_commit(self, commit_msg):
        self.call("add", ".")
        self.call("commit", "-F", "/dev/stdin", input=commit_msg)


    def create_pull_request(self, description, target_branch, publish, automerge):
        args = ["pr", "create", "--to", target_branch]
        if publish:
            args.append("--publish")
        if automerge:
            args.append("--merge")
        if description:
            args.extend(["-F", "/dev/stdin"])

        self.call(*args, input=description)


    def status(self):
        out = self.output("status", "--json")
        return json.loads(out)["status"]


    def info(self):
        out = self.output("info", "--json")
        return json.loads(out)


    def get_user(self):
        user = self.info().get("user_login")
        if user is None:
            raise KeyError('missing "user_login": '
                           'update arc client, remove arc store, remount')
        return user

    def fetch(self, arg):
        self.call("fetch", arg)


    def ls_tree(self, sha, subdir):
        def parse_entry(line):
            # mode SP type SP sha1 TAB name
            head, name = line.split('\t')
            mode, type, oid = head.split(' ')
            mode = int(mode, 8)
            assert type == 'blob'
            return mode, type, oid, name

        out = self.output('ls-tree', '-r', '{}:{}'.format(sha, subdir))
        return [parse_entry(l) for l in out.splitlines()]


    def git_hash_blob(self, oid):
        # arc uses different hash algo
        show_proc = self.popen('show', oid, stdout=subprocess.PIPE)
        out = subprocess.check_output(['git', 'hash-object', '--stdin'],
                                      stdin=show_proc.stdout)

        ret = show_proc.wait()
        if ret != 0:
            raise subprocess.CalledProcessError(ret, "arc show")

        return out.decode().strip()


    def git_ls_tree(self, sha, subdir):
        return [(mode, type, self.git_hash_blob(oid), name)
                for mode, type, oid, name in self.ls_tree(sha, subdir)]


def exit_with_error(msg):
    print(msg)
    exit(1)


def abs_path_to_git_repo():
    path_to_scripts = os.path.dirname(os.path.abspath(__file__))
    path_to_repo_root = os.path.join(path_to_scripts, os.pardir)

    return os.path.abspath(path_to_repo_root)


def make_sync_branch_name(origin_branch, target_branch):
    return ARC_SYNC_PREFIX + origin_branch + "-to-" + target_branch


def main():
    parser = argparse.ArgumentParser(
                    description="Sync changes on the current git branch with arc")
    parser.add_argument(
        "arcadia_path", type=str,
        help="Path to the local arcadia repository", nargs=1)
    parser.add_argument(
        "--arc-sync-branch", type=str,
        help="Arcadia branch to use for sync, defaults to bb-sync-$git-current-branch")
    parser.add_argument(
        "--arc-target-branch", type=str, default=ARC_MAIN_BRANCH,
        help="Target branch in arcadia (default: %(default)s)")
    parser.add_argument(
        "--pr-description", type=str,
        help="Description to use for the arcadia pull-request (if not yet created)")
    parser.add_argument(
        "--force-create-pr", action="store_true",
        help="Create a new arcadia pull-request even if sync branch already existed")
    parser.add_argument(
        "--pr-publish", action="store_true",
        help="Automatically publish new pull-requests")
    parser.add_argument(
        "--pr-automerge", action="store_true",
        help="Enable auto-merge for new pull-requests")
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Run without creating new branches/commits/PRs in the arcadia repo")
    args = parser.parse_args()

    arc = Arc(os.path.abspath(args.arcadia_path[0]))
    git = Git(abs_path_to_git_repo())

    if arc.status():
        exit_with_error("Detected uncommited changes in arcadia, "
                        "stash or commit before proceeding")

    git_current_branch = git.get_current_branch()

    arc_target_branch = args.arc_target_branch
    prefixed_target_branch = "arcadia/{}".format(arc_target_branch)

    if args.arc_sync_branch:
        arc_sync_branch = args.arc_sync_branch
    else:
        arc_sync_branch = make_sync_branch_name(git_current_branch,
                                                arc_target_branch)

    arc_sync_branch_remote = "users/{}/{}".format(arc.get_user(),
                                                  arc_sync_branch)

    arc_vhost_full_path = os.path.join(arc.repo_path, ARC_VHOST_PATH)

    print("Updating arc branch '{}'".format(arc_target_branch))
    arc.fetch(arc_target_branch)

    ls_tree = arc.git_ls_tree(prefixed_target_branch, ARC_VHOST_PATH)
    tree_hash = git.hash_tree(ls_tree)
    git_base = git.latest_commit_by_tree(tree_hash)
    git_commits = git.get_log(git_base)
    if not git_commits:
        print("'{}' in arc is up to date with '{}' in "
              "git".format(arc_target_branch, git_current_branch))
        exit(0)

    print("Going to apply commits:")
    for i, (sha, subj) in enumerate(git_commits):
        print("{:3d}\t{:.12s}\t{:s}".format(i, sha, subj))

    print("Checking out '{}' at '{}'".format(arc_sync_branch,
                                             prefixed_target_branch))
    if not args.dry_run:
        if arc.get_current_branch() != arc_sync_branch:
            arc.call("checkout", "-f", "-b", arc_sync_branch, "--no-track",
                     prefixed_target_branch)
        else:
            arc.call("reset", "--hard", prefixed_target_branch)

    for sha, subject in git_commits:
        print("Applying commit {:.12} ({})...".format(sha, subject))

        if args.dry_run:
            continue

        body = git.get_commit_body(sha)
        git.extract_commit(sha, arc_vhost_full_path)
        arc.add_changes_and_commit(body)

    print("Force pushing '{}' to '{}'...".format(arc_sync_branch,
                                                 arc_sync_branch_remote))
    if not args.dry_run:
        arc.call("push", "-f", "--set-upstream", arc_sync_branch_remote)

    if not arc.test("pr", "status") or args.force_create_pr:
        print("Creating a new arc pull request "
              "to '{}'...".format(arc_target_branch))
        if not args.dry_run:
            arc.create_pull_request(args.pr_description, arc_target_branch,
                                    args.pr_publish, args.pr_automerge)


if __name__ == "__main__":
    main()
