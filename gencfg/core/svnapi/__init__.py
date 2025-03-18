"""
    Python interface to svn. Somewhat analogous to git svn in gitapi.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import namedtuple, defaultdict
import re
from xml.etree import cElementTree
import time
import shutil

from config import ARCADIA_RELATIVE_REGEX
from gaux.aux_utils import run_command

# strip /trunk /tags/<tag name> and /branches/<branch name> from string
SUBDIR_PATTERN = "(?:(?:/trunk/arcadia/gencfg)|(?:/trunk/data/gencfg_db)|(?:/tags/gencfg/(?:.*?))|(?:/branches/gencfg/(?:.*?)))/(.*)"

GENCFG_TRUNK_DATA_PATH = "svn+ssh://arcadia.yandex.ru/arc/trunk/data/gencfg_db"
GENCFG_TRUNK_SRC_PATH = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/gencfg"

GENCFG_BRANCH_PREFIX = "svn+ssh://arcadia.yandex.ru/arc/branches/gencfg"
GENCFG_TAG_PREFIX = "svn+ssh://arcadia.yandex.ru/arc/tags/gencfg"


class SvnError(Exception):
    def __init__(self, *args, **kwargs):
        message, args = args[0], args[1:]
        super(SvnError, self).__init__(
            message.format(*args, **kwargs) if args or kwargs else message)


class NothingToCommitError(SvnError):
    pass


class PushRejectedError(SvnError):
    pass


class MergeConflictError(SvnError):
    pass


class ChangesDiscardedOnMerge(SvnError):
    pass


class _RepositoryLockedError(SvnError):
    pass


class InvalidGitLogMessage(SvnError):
    pass


_CommandResult = namedtuple("_CommandResult", ["stdout", "stderr", "status"])


def extract_watchers_from_message(message):
    watchers = re.findall("([a-zA-Z-0-9_]+)@", message)
    watchers = sorted(list(set(map(lambda x: x.strip(), sum(map(lambda x: x.split(','), watchers), [])))))

    from core.db import CURDB
    staff = {x.name for x in CURDB.users.get_users()}

    return filter(lambda x: x in staff, watchers)


def extract_jira_tasks_from_message(message):
    tasks = re.findall('([A-Z]+-[1-9][0-9]*)', message)
    return tasks


def extract_telechats(message):
    """
        Extract all telegram chats from commit message
    """
    return re.findall("([a-zA-Z-0-9_]+)#", message)


class SvnCommitMessage(object):
    """
        Class, corresponding to svn commit. Contains all information needed in diffbuilder and some other scripts
    """

    def __init__(self, xml):
        """
            Extract commit data from commit message.

            :param repo(core.svnapi.SvnRepository): current repository
            :param xml(cElementTree): svn log entry as xml tree
        """

        self.commit = int(xml.attrib["revision"])

        # extract data from message
        self.message = xml.find("msg").text.strip().encode('utf8')
        self.labels = re.findall('\[\s*(\w+)\s*\]', self.message)
        self.watchers = extract_watchers_from_message(self.message)
        self.jira_tasks = extract_jira_tasks_from_message(self.message)
        self.telechats = extract_telechats(self.message)

        # replace robot-gencfg by real owner
        self.author = xml.find("author").text
        if self.author == "robot-gencfg":
            m = re.match(".*\(commited by (.*)@\)$", self.message)
            if m:
                self.author = m.group(1)

        # date in xml returned by svn is always in UTC
        self.date = time.mktime(time.strptime(xml.find("date").text.partition('.')[0], "%Y-%m-%dT%H:%M:%S"))
        self.date -= time.timezone

        # get changed files
        self.modified_files = []
        for elem in xml.findall("paths/path"):
            if elem.attrib["kind"] == "file":
                fname = elem.text
                # strip /gencfgmain/trunk /gencfgmain/tags/<tag name> and /gencfgmain/branches/<branch name> from fname
                m = re.match(SUBDIR_PATTERN, fname)
                if m:
                    fname = m.group(1)
                    self.modified_files.append(fname)


class SvnRepository(object):
    def __init__(self, path=None, subdir=None, default_branch="trunk", verbose=False, temporary=False):
        self.path = os.getcwd() if path is None else path

        self.__default_branch = default_branch
        self.__verbose = verbose

        # some cached values
        self.current_tag = None
        self.current_tag_cached = False

        self.current_branch = None
        self.current_branch_cached = False

        self.current_commit = None
        self.current_commit_cached = False

        self.temporary = temporary

        if subdir is not None:
            self.subdir = subdir
        else:
            relative_url = self.svn_info_relative_url()
            m = re.match("\\^" + SUBDIR_PATTERN, relative_url)
            if m:
                self.subdir = m.group(1)
            else:
                self.subdir = None

    @classmethod
    def clone(cls, dest, repo_type, subdir=None, tag=None):
        """
            Clone repo at specified path and return corresponding SvnRepository object.

            :type dest: str
            :type repo_type: str
            :type tag: str

            :param dest: path on filesystem to clone repository to
            :param repo_type: one on ('full', 'db', 'balancer')
            :param tag: existing tag name (mutually exclusive with branch name)

            :return SvnRepository: svn repository cloned at path <path>
        """

        import utils.standalone.get_gencfg_repo

        utils.standalone.get_gencfg_repo.clone(dest, repo_type, tag=tag)

        return SvnRepository(dest, subdir=subdir)

    def is_db_repo(self):
        svn_url = self.svn_info_absolute_url()
        if svn_url == GENCFG_TRUNK_SRC_PATH:
            return False
        if svn_url == GENCFG_TRUNK_DATA_PATH:
            return True

        if svn_url.startswith(GENCFG_TAG_PREFIX):
            if svn_url.endswith('/db'):
                return True
            return False
        else:
            raise Exception('Unknown url <{}>'.format(svn_url))

    def checkout(self, tag):
        """
            Check out specified tag. Examples of ref are <stable-85-r55>

            :type tag: str

            :param tag: tag in repo (usually path to tag or branch)
            :return: nothin to return, all changes are made inplace.
        """

        if self.is_db_repo():
            if tag == 'trunk':
                checkout_url = GENCFG_TRUNK_DATA_PATH
            else:
                checkout_url = '{}/{}/db'.format(GENCFG_TAG_PREFIX, tag)
        else:
            SvnRepository(os.path.join(self.path, 'db')).checkout(tag)
            if tag == 'trunk':
                checkout_url = GENCFG_TRUNK_SRC_PATH
            else:
                checkout_url = '{}/{}'.format(GENCFG_TAG_PREFIX)

        print 'Checkout url {}'.format(checkout_url)
        self.command(["switch", "-q", "--ignore-ancestry", checkout_url])

    def describe_commit(self):
        """
            Returns a string that uniquely identifies current commit and which as human friendly as it's possible (tries
            to make it based on nearest tag name).
        """
        tree = self.svn_info_as_xml()

        commit = tree.find("entry/commit").attrib["revision"]

        url = self.svn_info_absolute_url()
        if url == GENCFG_TRUNK_DATA_PATH:
            return "(trunk, rev. %s)" % commit
        elif url.startswith(GENCFG_TAG_PREFIX):
            return "(tag %s, rev. %s)" % (url[len(GENCFG_TAG_PREFIX):], commit)
        elif url.startswith(GENCFG_BRANCH_PREFIX):
            print url[len(GENCFG_BRANCH_PREFIX):]
            return '(branch {}, rev {})'.format(url[len(GENCFG_BRANCH_PREFIX)+1:].partition('/')[0], commit)
        else:
            raise Exception("Relative url <%s> can not be parsed as branch or tag name" % url)

    def get_current_tag(self):
        """
            Returns current tag name or None if we not in tagged repo
        """

        if not self.current_tag_cached:
            url = self.svn_info_relative_url()
            if url.startswith("^/tags/gencfg/"):
                self.current_tag = url.split('/')[3]
            else:
                self.current_tag = None
            self.current_tag_cached = True
        return self.current_tag

    def get_current_branch(self):
        """
            Returns current branch name or None if we not in tagged repo
        """

        if not self.current_branch_cached:
            url = self.svn_info_relative_url()
            if url.startswith("^/branches/gencfg/"):
                self.current_branch = url.split('/')[3]
            else:
                self.current_branch = None
            self.current_branch_cached = True
        return self.current_branch

    def get_current_branch_or_tag(self):
        """
            Return current tag if tagged else current branch
        """

        result = self.get_current_tag()
        if result is None:
            result = self.get_current_branch()
        if result is None:
            result = "trunk"

        return result

    def default_branch(self):
        return self.__default_branch

    def add(self, paths):
        if paths:
            self.command(["add", "--parents"] + paths)

    def rm(self, paths):
        if paths:
            self.command(["rm", "--force"] + paths)

    def commit(self, message, paths=None, author=None, reset_changes_on_fail=False):
        """
            Commits the changes. Raises NothingToCommitError if there is nothing to commit.
        """

        del author

        try:
            # check if we have something to commit
            args = ["status", "-q"]
            if paths:
                args += paths
            result = self.command(args)
            if result.stdout == "":
                raise NothingToCommitError("There is nothing to commit in files <%s> to <%s>",
                                           ("None" if paths is None else ",".join(paths), self.path))

            # have non-empty diff, just commit
            args = ["commit", "-m", message]
            if paths:
                args += paths
            self.command(args)
        except SvnError:
            if reset_changes_on_fail:
                try:
                    self.reset_all_changes()
                except Exception:
                    pass
            raise

    def reset_all_changes(self, remove_unversioned=False):
        self.command(["revert", ".", "-R"])

        if remove_unversioned:
            for fname in self.get_changed_file_statuses(self.path)["unversioned"]:
                if fname == 'cache':  # do not remove cache
                    continue

                fname = os.path.join(self.path, fname)
                if os.path.isfile(fname):
                    os.remove(fname)
                else:
                    shutil.rmtree(fname)

    def sync(self, commit=None):
        """
            Syncs the local copy with remote. If force is True discards any local changes. Returns True if the local
            copy has been changed.

            :type commit: int

            :param commit: commit to update to (by default update to last commit)
        """

        last_commit = self.get_last_commit_id()

        args = ["update", "--non-interactive"]
        if commit is not None:
            args.extend(["--revision", str(commit)])
        self.command(args)

        self.current_commit_cached = False
        self.current_tag_cached = False
        self.current_branch_cached = False

        new_last_commit = self.get_last_commit_id()

        status_result = self.command(["status", "-q"])
        if len(filter(lambda x: x.startswith('C'), status_result.stdout.split('\n'))) > 0:
            # have conflict, have to discard all changes
            self.command(["revert", ".", "-R"])
            self.command(["update", "--non-interactive", "--revision", str(last_commit)])
            raise ChangesDiscardedOnMerge("Some changes has been discarded during merge.")

        return new_last_commit != last_commit

    def tags(self, timeout=None):
        """
            Return NON-SORTED list of tags from remote.
        """

        result = self.command(['ls', GENCFG_TAG_PREFIX], timeout=timeout)
        return map(lambda x: x.replace("/", "").strip(), result.stdout.split())

    def create_tag(self, tagname, message=None):
        if message is None:
            message = "Created tag <%s>" % tagname

        tagurl = os.path.join(GENCFG_TAG_PREFIX, tagname)

        # copy code
        self.command(['copy', GENCFG_TRUNK_SRC_PATH, tagurl, '-m', message])

        last_code_commit = self.get_last_commit_id(tagurl) - 1

        # copy data
        self.command(['copy', '{}@{}'.format(GENCFG_TRUNK_DATA_PATH, last_code_commit), os.path.join(tagurl, 'db'), '-m', message])

    def delete_tag(self, tagname, message=None):
        if message is None:
            message = 'Removed tag <%s>' % tagname
        self.command(['delete', '{}/{}'.format(GENCFG_TAG_PREFIX, tagname), '-m', message])

    def extract_commits_from_log(self, xmltree):
        commits = []
        for entry in xmltree.findall("logentry"):
            commits.append(SvnCommitMessage(entry))
            # description of merge commits can include list of nested logentry (revisions, commited in branch)
            # thus we have to add this commits here
            for branch_entry in entry.findall("logentry"):
                commits.append(SvnCommitMessage(branch_entry))

        return commits

    def get_last_commits(self, N, remote_url=None):
        """Return last N commits"""

        args = ['-l', str(N)]

        return self.extract_commits_from_log(self.log_command_xml(args, remote_url=remote_url))

    def get_commits_since(self, commit, remote_url=None, my_remote=False):
        """
            Get all commits since specified one. If my_remote is True, get commits from remote rather than from local
        """
        args = ["-r", "%s:HEAD" % commit]

        return self.extract_commits_from_log(self.log_command_xml(args, remote_url=remote_url, my_remote=my_remote))

    def get_commits_date_range(self, startts=None, endts=None, remote_url=None, my_remote=False):
        if startts is not None:
            start = "{%s}:" % time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(startts))
        else:
            start = ""
        if endts is not None:
            end = "{%s}" % time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(endts))
        else:
            end = "HEAD"
        revision_range = "%s%s" % (start, end)
        args = ["-r", revision_range]

        commits = self.extract_commits_from_log(
            self.log_command_xml(args, remote_url=remote_url, my_remote=my_remote))

        # we should filter commits by date because svn returns revision younger than startts
        if startts is not None:
            commits = filter(lambda x: x.date >= startts, commits)
        if endts is not None:
            commits = filter(lambda x: x.data <= endts, commits)

        return commits

    def get_commits_commit_range(self, startcommit=1, endcommit="HEAD", remote_url=None, my_remote=False):
        revision_range = "%s:%s" % (startcommit, endcommit)
        args = ["-r", revision_range]

        return self.extract_commits_from_log(self.log_command_xml(args, remote_url=remote_url, my_remote=my_remote))

    def get_commit(self, commit, remote_url=None, my_remote=False):
        args = ["-r", str(commit)]
        return self.extract_commits_from_log(self.log_command_xml(args, remote_url=remote_url, my_remote=my_remote))[0]

    def get_previous_commit(self, commit, remote_url=None, my_remote=False):
        return self.get_commit(commit - 1, remote_url=remote_url, my_remote=my_remote)

    def get_last_commit(self, url=None):
        """
            This function return core.svnapi.SvnCommitMessage object rather than just number.

            :type url: str

            :param url: If <url> is not None, get last commit from specified svn url, otherwise get commit from
            local svn info

            :return (core.svnapi.SvnCommitMessage): last commit
        """

        tree = self.svn_info_as_xml(url=url)

        return self.get_commit(int(tree.find("entry/commit").attrib["revision"]))

    def get_last_commit_id(self, url=None):
        if (url is not None) or (not self.current_commit_cached):
            tree = self.svn_info_as_xml(url=url)
            self.current_commit = int(tree.find("entry/commit").attrib["revision"])
            self.current_commit_cached = True

        return self.current_commit

    def get_commits_count(self, url=None):
        return self.get_last_commit_id(url=url)

    def svn_status_as_xml(self, path):
        args = ["status", "--xml", path]

        command_result = self.command(args)
        tree = cElementTree.fromstring(command_result.stdout)

        return tree

    def get_changed_file_statuses(self, path):
        """
            Return list of unversioned files at path. Return path names, relative to <path> argument

            :type path: str

            :param path: path to svn directory

            :return (dict of <str, list>): dict with file statuses types, like <unversioned>, <modified> with list of
            files with this status
        """

        tree = self.svn_status_as_xml(path)

        result = defaultdict(list)
        for entry in tree.findall('.*/entry'):
            wc_status = entry.find('wc-status')
            if wc_status is not None:
                file_status = wc_status.attrib.get('item', None)
                result[file_status].append(os.path.relpath(entry.attrib['path'], path))

        return result

    def has_file(self, path):
        info = self.svn_info_as_xml(url=path, ok_statuses=(0, 1))

        return len(info.findall('./entry')) > 0

    def ping(self):
        """
            Check if remote is still working. Needed for backend
        """

        try:
            self.command(["ls", "^/"])
            return True
        except SvnError:
            return False

    def svn_info_absolute_url(self):
        tree = self.svn_info_as_xml()
        return _normalize_url(tree.find("entry/url").text)

    def svn_info_relative_url(self, url=None):
        """
            Function returs relative url starting with ^
        """

        tree = self.svn_info_as_xml(url=url)
        myurl = tree.find("entry/url").text
        m = re.match(ARCADIA_RELATIVE_REGEX, myurl)
        if not m:
            raise Exception("Url <%s> does not start satisfy regex <%s>" % (myurl, ARCADIA_RELATIVE_REGEX))
        myurl = "^" + m.group(1)

        return myurl

    def svn_info_as_xml(self, url=None, timeout=None, **kwargs):
        args = ["info", "--xml"]
        if url is not None:
            args.append(url)

        command_result = self.command(args, timeout=timeout, **kwargs)

        return cElementTree.fromstring(command_result.stdout)

    def log_command_xml(self, args, verbose=True, remote_url=None, my_remote=False, no_merge_history=False, **kwargs):
        """
            Function invoke svn log and return result as xml.etree

            :type args: list[str]
            :type verbose: bool
            :type remote_url: str
            :type my_remote: bool

            :param args: list of arguments to pass to svn command
            :param verbose: If True add some verbose output
            :param remote_url: get svn log for specified url rather then for local cloned repository. Mutually
            exclusive with <my_remote == True>
            :param my_remote: If True ask for remote instead of asking local repository. Mutually exclusive with
            <remote_url != None>
        """

        if no_merge_history:
            args = ["log", "--verbose", "--xml"] + args
        else:
            args = ["log", "-g", "--verbose", "--xml"] + args

        if my_remote is True:
            args.append(self.svn_info_as_xml().find("entry/url").text)
        elif remote_url is not None:
            args.append(remote_url)

        result = self.command(args, verbose=verbose, **kwargs)

        try:
            return cElementTree.fromstring(result.stdout)
        except Exception as e:
            print 'Got <{}> exception with data <{}>'.format(e.__class__, result.stdout)

    def command(self, args, retries=5, verbose=True, timeout=60, **kwargs):
        retry_timeout = [1, 5, 10, 20, 40]
        if verbose and self.__verbose:
            print("{}: {}".format(self.path, " ".join(args)))

        for attempt in xrange(retries):
            try:
                return _cmd(args, cwd=self.path, timeout=timeout, **kwargs)
            except SvnError:
                if attempt == retries - 1:
                    raise

                # try to cleanup database, because exception could be due to somthing was locked
                try:
                    _cmd(["cleanup"], cwd=self.path, timeout=3)
                except Exception:
                    pass

                time.sleep(retry_timeout[min(len(retry_timeout) - 1, attempt)])

    def __del__(self):
        if self.temporary:
            shutil.rmtree(self.path)


def _cmd(args, cwd=None, ok_statuses=(0,), error_handler=None, timeout=None):
    returncode, stdout, stderr = run_command(["svn", "--non-interactive"] + args, raise_failed=False, cwd=cwd,
                                             close_fds=True, timeout=timeout)

    result = _CommandResult(stdout, stderr, returncode)

    if result.status not in ok_statuses:
        if error_handler is not None:
            error_handler(result)

        raise SvnError("Command <{}> at cwd <{}> failed. Status: <{}>. Error message: <{}>. Out: <{}>.",
                       " ".join(["svn"] + args),
                       cwd,
                       result.status,
                       _text_to_line(result.stderr),
                       _text_to_line(result.stdout)
        )

    return result


def _normalize_url(data):
    proto, delim, url = data.partition('://')

    m = re.match('^[a-zA-Z0-9_-]+@(.*)', url)
    if m:
        url = m.group(1)

    return '{}{}{}'.format(proto, delim, url)


def _text_to_line(text):
    return re.sub(r"\s*\n\s*", " ", text).strip()
