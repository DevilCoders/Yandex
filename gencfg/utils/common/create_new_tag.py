#!/skynet/python/bin/python
"""
    This script creates new tag for one of the following things:
        - gencfg configs;
        - backed;
        - balancer configs.

    Fistly, sandbox task is started. Upon task completion some additional actions can be done (like write tag info in remote databases)
"""

import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

# FIXME: copied from balancer config tag creation

import time
import traceback
import xmlrpclib
import logging

import gencfg
from core.argparse.parser import ArgumentParserExt
import config
import custom_generators.balancer_gencfg.balancer_config
import web_shared.service_config
import gaux.aux_utils
from gaux.aux_colortext import red_text
from core.settings import SETTINGS


class EGencfgPrjs(object):
    CONFIGS = "configs"
    BACKEND = "backend"
    BALANCER_CONFIGS = "balancer_configs"
    ALL = [CONFIGS, BACKEND, BALANCER_CONFIGS]


def get_parser():
    parser = ArgumentParserExt(
        description="Create new tag for one of gencfg things to be released (all configs/balancer configs/backends)")
    parser.add_argument("-s", "--run-sandbox", type=str, required=True,
                        choices=["yes", "no"],
                        help="Obligatory. Create sandbox task: \"yes\" or \"no\"")
    parser.add_argument("-t", "--rewrite-tag", type=str, required=True,
                        choices=["yes", "no"],
                        help="Obligatory. Rewrite last tag: \"yes\" or \"no\"")
    parser.add_argument("-p", "--prj", type=str, required=True,
                        choices=EGencfgPrjs.ALL,
                        help="Obligatory. Prj to build tag for")
    parser.add_argument("-c", "--custom-tag-name", type=str, default=None,
                        help="Optional. Custom tag name")
    parser.add_argument("--notify-user", type=str, dest="notify_user",
                        default=os.environ.get("USER", "unknown"),
                        help="notify specified user (if sandbox task will be finished or failed) (by default notify user who run the script)")

    return parser


def normalize(options):
    options.run_sandbox = True if options.run_sandbox == "yes" else False

    options.rewrite_tag = True if options.rewrite_tag == "yes" else False
    if options.rewrite_tag:
        print red_text("Option --rewrite-tag is dangerous. If something goes wrong, tag will be removed")


def _print_error(message):
    print >> sys.stderr, message


def _log_error(message):
    import logging
    logging.error(message)


def new_tag_name(options, repo=None):
    if repo is None:
        repo = gaux.aux_utils.get_main_repo(verbose=True)

    if options.prj == EGencfgPrjs.CONFIGS:
        tag_prefix = "stable-{}-r".format(config.BRANCH_VERSION)
    elif options.prj == EGencfgPrjs.BACKEND:
        tag_prefix = web_shared.service_config.TAG_PREFIX
    elif options.prj == EGencfgPrjs.BALANCER_CONFIGS:
        tag_prefix = custom_generators.balancer_gencfg.balancer_config.TAG_PREFIX
    else:
        raise Exception("Unknown prj type <%s>" % options.prj)

    last_release = gaux.aux_utils.get_last_release(repo, tag_prefix)

    if options.rewrite_tag:
        if last_release == 0:
            raise Exception("No last tag to rewrite")

        tag_name = tag_prefix + str(last_release)
    else:
        tag_name = tag_prefix + str(last_release + 1)

    return tag_name


def create_new_tag(options, error_handler, use_sandbox_api=False):
    repo = gaux.aux_utils.get_main_repo(verbose=True)

    if options.custom_tag_name is None:
        tag_name = new_tag_name(options, repo)
    else:
        tag_name = options.custom_tag_name

    print "Creating new tag: {}".format(tag_name)

    repo_tag_created = False
    try:
        if options.rewrite_tag:
            repo.delete_tag(tag_name)

        repo.create_tag(tag_name)
        repo_tag_created = True

        if options.run_sandbox:
            # create sandbox task
            if options.prj == EGencfgPrjs.CONFIGS:
                task_type = "BUILD_CONFIG_GENERATOR"
                task_ctx = {
                    "tag": tag_name,
                    "build_bundle": False,
                    "generate_diff_to_prev": True,
                    "notify_if_finished": options.notify_user,
                    "notify_if_failed": options.notify_user,
                }
            elif options.prj == EGencfgPrjs.BACKEND:
                task_type = "BUILD_CONFIG_GENERATOR_SERVICE"
                task_ctx = {
                    "tag": tag_name,
                    "notify_if_finished": options.notify_user,
                    "notify_if_failed": options.notify_user,
                }
            elif options.prj == EGencfgPrjs.BALANCER_CONFIGS:
                task_type = "BUILD_BALANCER_CONFIG_GENERATOR"
                task_ctx = {
                    "tag": tag_name,
                    "notify_if_finished": options.notify_user,
                    "notify_if_failed": options.notify_user,
                }
            else:
                raise Exception("Unknown prj type <%s>" % options.prj)

            if use_sandbox_api:  # RX-452
                import sdk2
                import sandbox.common.types.task as ctt

                task_class = sdk2.Task[task_type]
                sub_task = task_class(task_class.current, owner='GENCFG', descr='Building {}'.format(tag_name), **task_ctx)
                wait_task = sub_task.save().enqueue()
                raise sdk2.WaitTask([wait_task], ctt.TaskStatus.FINISHED, wait_all=True)
            else:
                proxy = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc', allow_none=True,
                                              transport=gaux.aux_utils.OAuthTransport(config.get_default_oauth()))
                params = {
                    "type_name": task_type,
                    "owner": "GENCFG",
                    "descr": "Building {}".format(tag_name),
                    "priority": ("SERVICE", "HIGH"),
                    "arch": "linux_ubuntu_12.04_precise",  # command "getent group yandex_mnt" works only on precise
                    "ctx": task_ctx,
                    "ram": 75000,
                }
                task_id = proxy.createTask(params)

                print "Waiting sandbox task: %d (https://sandbox.yandex-team.ru/task/%d/view)" % (task_id, task_id)
                wait_till = time.time() + 3600
                while time.time() < wait_till:
                    try:
                        status = proxy.getTaskStatus(task_id)
                    except:
                        time.sleep(1)
                        continue

                    if status not in ['ENQUEUED', 'ENQUEUING', 'EXECUTING', 'PREPARING', 'FINISHING', 'TEMPORARY', 'ASSIGNED', 'DRAFT']:
                        if status == 'SUCCESS':
                            break
                        raise Exception("Task %s%d exited with status %s" % (SETTINGS.services.sandbox.http.tasks.url, task_id, status))

                    time.sleep(1)
                if time.time() > wait_till:
                    raise Exception("Task %s%d did not finished" % (SETTINGS.services.sandbox.http.tasks.url, task_id))
    except Exception as error:
        error_handler("Got an error: {}".format(error))
        traceback.print_exc()

        error_handler("Reverting all changes.")

        if repo_tag_created:
            try:
                repo.delete_tag(tag_name)
            except Exception as e:
                error_handler("Failed to revert creation of tag {}: {}".format(tag_name, e))

        try:
            repo.reset_all_changes()
        except Exception as e:
            error_handler("Failed to rollback to {} commit on DB: {}".format(db_fallback_ref, e))

        raise


def main():
    options = get_parser().parse_cmd()

    normalize(options)

    create_new_tag(options, _print_error)


def jsmain(d):
    options = get_parser().parse_json(d)

    create_new_tag(options, _log_error, use_sandbox_api=True)


if __name__ == '__main__':
    try:
        main()
    except Exception:
        sys.exit(os.EX_SOFTWARE)
