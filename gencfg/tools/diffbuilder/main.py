#!/skynet/python/bin/python
# coding: utf8

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

VERSION = "0.2.4"

import re
from argparse import ArgumentParser
from collections import OrderedDict
import functools
import json
from collections import defaultdict

import gencfg
import core.argparse.types as argparse_types
from core.db import CURDB, DB
from utils.standalone import get_gencfg_repo
from utils.common import show_nanny_services_groups
from core.settings import SETTINGS
from gaux.aux_colortext import red_text
from gaux.aux_repo import get_tags_by_tag_pattern
from gaux.aux_staff import unwrap_dpts
from core.svnapi import SvnRepository
import jsonschema

from diffbuilder_types import EDiffTypes, TMyJsonEncoder
from groupdiffer import GroupDiffer
from tierdiffer import TierDiffer
from commitdiffer import TCommitDiffer
from searcherlookupdiffer import SearcherlookupDiffer
from hwdiffer import HwDiffer
from intlookupdiffer import IntlookupDiffer
from diffbuilder_utils import construct_repo_from_name

DIFFERS = OrderedDict([
    ('commit', lambda options: TCommitDiffer(options.oldrepo, options.olddb, options.newrepo, options.newdb,
                                             options.commit_filter).get_diff()),
    ('group', lambda options: get_something_diff(options.olddb, options.newdb, lambda db: dict(
        map(lambda x: (x.card.name, x), db.groups.get_groups())), GroupDiffer)),
    ('tier', lambda options: get_something_diff(options.olddb, options.newdb,
                                                lambda db: dict(map(lambda x: (x.name, x), db.tiers.get_tiers())),
                                                TierDiffer)),
    ('hw', lambda options: HwDiffer(options.olddb, options.newdb).get_diff()),
    ('intlookup', lambda options: get_something_diff(options.olddb, options.newdb, lambda db: dict(
        map(lambda x: (x.file_name, x), db.intlookups.get_intlookups())), IntlookupDiffer)),
])

REPORT_TYPES = ["text", "json"]


def parse_cmd():
    parser = ArgumentParser(description="Get diff between arbitrary tags")
    parser.add_argument("-o", "--old-tag", dest="old_tag", type=str, default=None,
                        help="Optional. Path or svn url to old tag")
    parser.add_argument("-n", "--new-tag", dest="new_tag", type=str, default=None,
                        help="Optional. Path or svn url to new tag")
    parser.add_argument("-d", "--differs", dest="differs",
                        type=functools.partial(argparse_types.comma_list_ext, deny_repeated=True,
                                               allow_list=DIFFERS.keys() + ["all"]),
                        default=["all"],
                        help="Optional. List of differs to apply (some of <%s> or <all> to specify all differs" % ",".join(
                            DIFFERS.keys()))
    parser.add_argument("--diff-specific-groups", type=argparse_types.comma_list, default=None,
                        help="Optional. Diff only specified groups (overrides --differ argument)")
    parser.add_argument("--commit-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Lambda function to filter commits")
    parser.add_argument("-p", "--diff-with-previous-tag", dest="diff_with_previous_tag", action="store_true",
                        default=False,
                        help="Generate diff with last tag")
    parser.add_argument("--tag-pattern", type=str, default="^stable-(\d+)/r(\d+)$",
                        help="Tag pattern for tags we are interested in")
    parser.add_argument("-u", "--diff-uncommited", dest="diff_uncommited", action="store_true", default=False,
                        help="Generate diff with last commited tag in db")
    parser.add_argument("-e", "--diff-specific-commit", type=int, default=None,
                        help="Generate diff for specific commit")

    parser.add_argument("-m", "--mail-dir", dest="mail_dir", type=str,
                        help="Optional. Path to directory with mail")
    parser.add_argument("-s", "--st-dir", type=str,
                        help="Optional. Path to directory with startrek comments")
    parser.add_argument("--nanny-filename", type=str,
                        help="Optional. File to save diff for nanny")
    parser.add_argument("--gencfg-announce-filename", type=str,
                        help="Optional. File to save announce to gencfg")
    parser.add_argument("-q", "--quiet", action="store_true", default=False,
                        help="Optional. More quiet output")
    parser.add_argument("--report-type", default="text",
                        choices=REPORT_TYPES,
                        help="Optional. Formatter for output type (currently only <text> and <json> are supported")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if (options.diff_with_previous_tag is True) + (options.diff_uncommited is True) + (options.old_tag is not None) + \
            (options.diff_specific_commit is not None) != 1:
        raise Exception("You must specify exactly one of <--diff-with-previous-tags> <--diff-uncommited> <--old-tag and --new-tag> <--diff-specific-commit> options")
    if "all" in options.differs and len(options.differs) != 1:
        raise Exception("<all> differ is incompatable with any other differ")
    if options.diff_uncommited and len({"commitdb", "commitmain", "commitbalancer"} & set(options.differs)) > 0:
        raise Exception("Option --diff-uncommited is incompatable with differs <commitdb, commitmain, commitbalancer>")

    if options.differs == ["all"]:
        options.differs = DIFFERS.keys()

    if options.diff_with_previous_tag:
        if options.new_tag is None:
            newrepo = SvnRepository(os.path.join(os.path.dirname(__file__), '..', '..'))
        else:
            newrepo = construct_repo_from_name(options.new_tag)

        # find previous tag
        mytags = get_tags_by_tag_pattern(options.tag_pattern, newrepo)
        newtag = newrepo.get_current_tag()
        oldrepo_index = 0
        oldrepo = None
        if newtag is None:
            oldrepo_index = len(mytags) - 1
        else:
            oldrepo_index = mytags.index(newtag) - 1

        for ind in reversed(range(oldrepo_index + 1)):
            repo = construct_repo_from_name("tag@%s" % mytags[ind])
            if repo.has_file('db'):
                oldrepo = repo
                break
        if oldrepo is None:
            raise Exception("Can't find previous tag")
    elif options.diff_uncommited:
        newrepo = SvnRepository(os.path.join(os.path.dirname(__file__), '..', '..'))
        oldrepo = SvnRepository(
            get_gencfg_repo.api_main(repo_type='full', revision=newrepo.get_last_commit().commit, load_db_cache=True).path, temporary=True)
    elif options.diff_specific_commit:
        newrepo = SvnRepository(get_gencfg_repo.api_main(repo_type='full', revision=options.diff_specific_commit, load_db_cache=True).path,
                                temporary=True)
        oldrepo = SvnRepository(
            get_gencfg_repo.api_main(repo_type='full', revision=options.diff_specific_commit - 1, load_db_cache=True).path, temporary=True)
    else:  # specified both new and old tags
        if options.new_tag is None or options.old_tag is None:
            raise Exception("You must specify both --new-tag and --old-tag option when omit --diff-with-previous-tags")

        newrepo = construct_repo_from_name(options.new_tag)
        oldrepo = construct_repo_from_name(options.old_tag)

    options.newrepo = newrepo
    options.newdb = DB(os.path.join(newrepo.path, 'db'))
    options.oldrepo = oldrepo
    options.olddb = DB(os.path.join(oldrepo.path, 'db'))

    return options


def get_something_diff(olddb, newdb, obj_selector, DifferClass):
    # create group pairs
    oldobjs = obj_selector(olddb)
    newobjs = obj_selector(newdb)
    objpairs = []
    for name in sorted(list(set(oldobjs.keys() + newobjs.keys()))):
        newobj = newobjs.get(name, None)
        oldobj = oldobjs.get(name, None)
        objpairs.append((oldobj, newobj))
    obj_differs = map(lambda (x, y): DifferClass(x, y, olddb, newdb), objpairs)

    result = map(lambda x: x.get_diff(), obj_differs)
    result = filter(lambda x: x.status is True, result)
    return result


def get_diff_for_user(options, diff_results, report_type, user=None):
    """
        This function generate text/json diff for specific user (if user == None, generated all diff)
    """

    assert report_type in ["text", "json"], "Unknown report type %s" % report_type

    result_entries = []
    for diff_entry in diff_results:
        if not diff_entry.status:
            continue
        if user is not None and user not in unwrap_dpts(diff_entry.watchers, suppress_missing=True, db=options.newdb):
            continue

        if report_type == 'text':
            text_item = '{}'.format(diff_entry.report_text())
            text_item = unicode(text_item, 'utf8') if not isinstance(text_item, unicode) else text_item
        elif report_type == 'json':
            text_item = diff_entry.report_json(as_string=False)
        result_entries.append(text_item)

    if report_type == "json":
        sandbox_task_id = os.environ.get('SANDBOX_TASK_ID')
        if sandbox_task_id is not None:
            sandbox_url = "https://sandbox.yandex-team.ru/sandbox/tasks/view?task_id=%s\n" % sandbox_task_id
        else:
            sandbox_url = None

        result = {
            "diff_props": {
                "version": VERSION,
                "oldtag": options.olddb.get_repo().get_current_branch_or_tag(),
                "newtag": options.newdb.get_repo().get_current_branch_or_tag(),
                "sandbox_url": sandbox_url,
            },
            "diff_result": result_entries,
        }

        result = json.dumps(result, cls=TMyJsonEncoder)

    elif report_type == "text":
        result = u''
        if not options.quiet:
            result += u'Differ v%s\n' % VERSION
            result += u'Diff from "%s" to "%s"\n' % (
                options.oldrepo.get_current_tag(),
                options.newrepo.get_current_tag(),
            )

            sandbox_task_id = os.environ.get('SANDBOX_TASK_ID')
            if sandbox_task_id is not None:
                result += u'Generated in task: https://sandbox.yandex-team.ru/sandbox/tasks/view?task_id=%s\n' % sandbox_task_id

            if user is None:
                all_watchers = list(set(sum(map(lambda x: x.watchers, diff_results), [])))
                result += u'All watchers: %s\n' % ', '.join(all_watchers)
            result += u'\n'

        # FIX UnicodeDecodeError
        result = result.decode('ascii').encode('utf8')
        result += (u'\n'.join(result_entries)).encode('utf8')

    else:
        raise Exception("Unknown report_type <%s>" % report_type)

    return result


def write_mail(options, diff_results, report_type, user=None):
    generated_diff = get_diff_for_user(options, diff_results, report_type, user=user)

    if options.mail_dir is None:
        print generated_diff
    else:
        if user is None:
            user = "ALL"
        with open(os.path.join(options.mail_dir, '%s.mail' % user), 'w') as f:
            f.write(generated_diff)


def main(options):
    # get results for all differs
    diff_results = []

    if options.diff_specific_groups is not None:
        for groupname in options.diff_specific_groups:
            diff_results.append(GroupDiffer(options.olddb.groups.get_group(groupname, raise_notfound=False),
                                            options.newdb.groups.get_group(groupname, raise_notfound=False)
                                            ).get_diff()
                                )
    else:
        for differ in options.differs:
            diff_results.extend(DIFFERS[differ](options))

    # write output files based on content of diff_results
    all_watchers = list(set(sum(map(lambda x: x.watchers, diff_results), [])))

    # convert staff/abc groups to owners
    all_watchers = unwrap_dpts(all_watchers, suppress_missing=True, db=options.newdb)

    write_mail(options, diff_results, options.report_type, user=None)
    if options.mail_dir is not None:
        for watcher in all_watchers:
            write_mail(options, diff_results, options.report_type, user=watcher)

    if options.st_dir is not None:
        all_tasks = set(sum(map(lambda x: x.tasks, diff_results), []))
        for task in all_tasks:
            task_commits = map(lambda x: x.report_text(), filter(lambda x: task in x.tasks, diff_results))
            with open(os.path.join(options.st_dir, task), 'w') as f:
                f.write('\n\n'.join(task_commits))

    if options.nanny_filename is not None:
        with open(options.nanny_filename, "w") as f:
            group_diff_results = filter(lambda x: x.diff_type == EDiffTypes.GROUP_DIFF, diff_results)
            nanny_diff = get_diff_for_user(options, group_diff_results, "json", user=None)

            f.write(nanny_diff)

    if options.gencfg_announce_filename is not None:
        with open(options.gencfg_announce_filename, "w") as f:
            announce_diffs = filter(lambda x: x.diff_type == EDiffTypes.COMMIT_DIFF and "gencfg" in x.telechats, diff_results)
            f.write("\n\n".join(map(lambda x: x.report_text(), announce_diffs)))


def realmain():
    options = parse_cmd()

    main(options)


if __name__ == '__main__':
    realmain()
