#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import shutil
import re
from argparse import ArgumentParser
import tempfile
import time
import xmlrpclib

import gencfg
from core.gitapi import GitRepository
import gaux.aux_utils


def parse_cmd():
    user = os.environ.get("USER", "unknown")

    parser = ArgumentParser(description="Create tag in arbitrary git repository")
    parser.add_argument("-r", "--repo", type=str, required=True,
                        help="Obligatory. Repo name")
    parser.add_argument("-b", "--branch", type=int, default=None,
                        help="Optional. Branch number (otherwise use branch from last tag")
    parser.add_argument("-t", "--rewrite-tag", action="store_true", default=False,
                        help="Optional. Rewrite last tag insted of creating new one")
    parser.add_argument("-s", "--sandbox-task", type=str, default=None,
                        help="Optional. Create specified sandbox task")
    parser.add_argument("--owner", type=str, default=user,
                        help="Optional. Owner of sandbox task")
    parser.add_argument("--oauth", type=str, default='68ab90694e784523ac7849796c09f344',
                        help="Optional. OAuth token to access sandbox")
    parser.add_argument("--notify-user", type=str, default=user,
                        help="Optional. Notify <NOTIFY_USER> if sandbox task will be finished or failed")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


def new_tag_name(options, repo):
    TAG_PATTERN = "stable-%s/r%s"

    tags = []
    for tag in repo.tags():
        m = re.match(TAG_PATTERN % ("(\d+)", "(\d+)"), tag)
        if m:
            tags.append((int(m.group(1)), int(m.group(2))))

    if options.branch:
        current_branch = options.branch
    elif len(tags) > 0:
        current_branch = max(map(lambda (b, t): b, tags))
    else:
        current_branch = 1

    tags = filter(lambda (branch, tag): branch == current_branch, tags)
    if len(tags):
        current_tag = max(map(lambda (b, t): t, tags)) + 1
    else:
        current_tag = 1

    return TAG_PATTERN % (current_branch, current_tag)


def main(options):
    temppath = tempfile.mkdtemp(prefix="somerepo-", dir="/var/tmp")
    print temppath

    main_repo_tag_created = False
    main_repo_tag_pushed = False

    try:
        main_repo = GitRepository.clone(temppath, options.repo)
        tag_name = new_tag_name(options, main_repo)

        print "Creating tag %s" % tag_name

        if options.rewrite_tag:
            if not gaux.aux_utils.prompt("Rewrite tag %s?"):
                print "New tag not created..."
                shutil.rmtree(temppath)
                return

        main_repo.create_tag(tag_name)
        main_repo_tag_created = True
        main_repo.push_tag(tag_name)
        main_repo_tag_pushed = True

        if options.sandbox_task is not None:
            proxy = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc', allow_none=True,
                                          transport=gaux.aux_utils.OAuthTransport(options.oauth))
            params = {
                "type_name": options.sandbox_task,
                "owner": options.owner,
                "descr": "Building {}".format(tag_name),
                "priority": ("SERVICE", "HIGH"),
                "arch": "linux_ubuntu_12.04_precise",  # command "getent group yandex_mnt" works only on precise
                "ctx": {
                    "tag": tag_name,
                    "notify_if_finished": options.notify_user,
                    "notify_if_failed": options.notify_user,
                },
            }

            task_id = proxy.createTask(params)

            print "Waiting sandbox task: %d" % task_id
            wait_till = time.time() + 2400
            while time.time() < wait_till:
                status = proxy.getTaskStatus(task_id)
                if status not in ['ENQUEUED', 'ENQUEUING', 'EXECUTING', 'PREPARING', 'FINISHING']:
                    if status == 'SUCCESS':
                        break
                    raise Exception("Task %d exited with status %s" % (task_id, status))
                time.sleep(1)
            if time.time() > wait_till:
                raise Exception("Task %d did not finished" % task_id)
    except:
        if main_repo_tag_created:
            main_repo.delete_tag(tag_name, push=main_repo_tag_pushed)

        shutil.rmtree(temppath)
        raise


if __name__ == '__main__':
    options = parse_cmd()

    main(options)
