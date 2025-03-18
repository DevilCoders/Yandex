import re
from collections import OrderedDict
import os

from core.svnapi import SvnRepository

from diffbuilder_types import EDiffTypes, TDiffEntry

DEVELOPERS = ["kimkim", "robot-gencfg"]


class TCommitDiffEntry(TDiffEntry):
    def __init__(self, diff_type, status, watchers, tasks, telechats, generated_diff, props=None):
        super(TCommitDiffEntry, self).__init__(diff_type, status, watchers, tasks, telechats, generated_diff, props=props)

    def report_text(self):
        pretty_commit = "Commit %s (by %s@)" % (self.generated_diff['commit'], self.generated_diff['author'])
        if len(self.generated_diff['modified_groups']) > 0:
            pretty_commit = "%s [modifies %s]" % (pretty_commit, ", ".join(self.generated_diff['modified_groups']))

        d = {
            pretty_commit: {
                "Message": self.generated_diff["message"],
            }
        }
        return self.recurse_report_text(d)


class TCommitDiffer(object):
    def __init__(self, oldrepo, olddb, newrepo, newdb, commit_filter):
        self.oldrepo = oldrepo
        self.olddb = olddb
        self.newrepo = newrepo
        self.newdb = newdb
        self.commit_filter = commit_filter

    def extract_extra_from_commit(self, commit):
        """
            Function, which tries to extract extra information from commit. Should be overloaded in descendants.

            :param commit: Commit description (of type core.svnapi.SvnCommitMessage)
            :return: <list of extra watcher> <list of modified groups>
        """

        modified_groups = []
        # FIXME: uncomment as soon as possible
        # modified_files = commit.modified_files
        # modified_groups = []
        # for fname in modified_files:
        #     m = re.search('db/groups/([^/]*)/([^.]*).*', fname)
        #     if m and m.group(2) != 'card':
        #         modified_groups.append(m.group(2))

            # for every intlookup get corresponding groups
        #     m = re.search('db/intlookups/(.*)', fname)
        #     if m:
        #         iname = m.group(1)
        #         if self.olddb.intlookups.has_intlookup(iname):
        #             modified_groups.extend(self.olddb.intlookups.get_linked_groups(iname))
        #         if self.newdb.intlookups.has_intlookup(iname):
        #             modified_groups.extend(self.newdb.intlookups.get_linked_groups(iname))
        # modified_groups = list(set(modified_groups))

        extra_watchers = []
        for gname in modified_groups:
            if self.olddb.groups.has_group(gname):
                group = self.olddb.groups.get_group(gname)
                extra_watchers.extend(group.card.owners + group.card.watchers)
            if self.newdb.groups.has_group(gname):
                group = self.newdb.groups.get_group(gname)
                extra_watchers.extend(group.card.owners + group.card.watchers)
        extra_watchers = list(set(extra_watchers))

        return extra_watchers, modified_groups

    def get_diff(self):
        result = []

        commits = []

        for subpath in ('.', './db'):
            sub_oldrepo = SvnRepository(os.path.join(self.oldrepo.path, subpath))
            sub_newrepo = SvnRepository(os.path.join(self.newrepo.path, subpath))

            oldrepo_commit = sub_oldrepo.get_last_commit()
            newrepo_commit = sub_newrepo.get_last_commit()

            if newrepo_commit.commit > oldrepo_commit.commit:
                commits.extend(sub_newrepo.get_commits_commit_range(oldrepo_commit.commit + 1, newrepo_commit.commit))

        p = re.compile(".*NORELEASE.*", re.IGNORECASE)
        for commit in commits:
            if len(commit.jira_tasks) == 0:
                if re.match(p, commit.message) is not None:
                    if commit.author in DEVELOPERS:
                        continue
            if commit.message.startswith("Merge branch"):
                continue
            if not self.commit_filter(commit):  # apply filter, specified from command line
                continue

            # generate kinda json diff
            extra_watchers, modified_groups = self.extract_extra_from_commit(commit)
            jsdiff = OrderedDict([
                ("commit", commit.commit),
                ("author", commit.author),
                ("message", commit.message),
                ("modified_groups", modified_groups),
            ])

            diff_entry = TCommitDiffEntry(EDiffTypes.COMMIT_DIFF, True, [commit.author] + commit.watchers + extra_watchers,
                                          commit.jira_tasks, commit.telechats, jsdiff)
            result.append(diff_entry)
        return result
