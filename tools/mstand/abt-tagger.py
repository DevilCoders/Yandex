#!/usr/bin/env python2.7
# coding=utf-8
import docopt
import logging

import yaqutils.misc_helpers as umisc
import yaqutils.requests_helpers as urequests

TARGET_TYPE_TASK = 0
TARGET_TYPE_OBSERVATION = 1
TARGET_TYPE_EXPERIMENT = 2

TT_TO_STR = {
    TARGET_TYPE_TASK: "task",
    TARGET_TYPE_OBSERVATION: "observation",
    TARGET_TYPE_EXPERIMENT: "experiment",
}

USAGE = """
A script to tag things.
Run with --add/--delete to add/delete tags. You can add or delete multiple tags in one go.
Run without any --add/--delete options to show currently set tags.

Usage:
  abt-tagger.py (-v ...) [options] obs <obs-id> [(--add=<tag-name> | --delete=<tag-name>)]...
  abt-tagger.py (-v ...) [options] exp <testid> [(--add=<tag-name> | --delete=<tag-name>)]...
  abt-tagger.py (-v ...) [options] task <task-id> [(--add=<tag-name> | --delete=<tag-name>)]...
  abt-tagger.py (-v ...) [options] search <tag-name>

  abt-tagger.py -h | --help

Options:
  -a TAG, --add=TAG                   Add a tag.
  -d TAG, --delete=TAG                Delete a tag.
  -h, --help                          Show this screen.
  -v, --verbose                       Use verbose logging.
  -u HOSTNAME, --api-host=HOSTNAME    Use specified ABT instance [default: ab.yandex-team.ru].
  --allow-editing-production-data     Allow changing data on production instance (ab.yandex-team.ru).
"""


def find_tags(api_host, params):
    return urequests.strict_request("GET", "http://" + api_host + "/api/tag", params=params).json()


def add_tag(api_host, target_type, target_id, tag_name):
    urequests.strict_request("POST", "http://" + api_host + "/api/tag", data={
        "target_type": target_type,
        "target_id": target_id,
        "name": tag_name
    })
    logging.info("Added tag %s to %s %s", tag_name, TT_TO_STR[target_type], target_id)


def del_tag(api_host, target_type, target_id, tag_name):
    params = {
        "target_type": target_type,
        "target_id": target_id
    }

    if tag_name != "*":
        params["name"] = tag_name

    tags = find_tags(api_host, params)

    if not tags:
        logging.warning("No tag %s is set on %s %s", tag_name, TT_TO_STR[target_type], target_id)
        return

    for tag in tags:
        urequests.strict_request("DELETE", "http://" + api_host + "/api/tag/" + tag["id"])
        logging.info("Removed tag %s from %s %s", tag["name"], TT_TO_STR[target_type], target_id)


def view_tags(api_host, target_type, target_id):
    tags = find_tags(api_host, {
        "target_type": target_type,
        "target_id": target_id
    })

    if not tags:
        logging.warning("No tags are set on %s %s", TT_TO_STR[target_type], target_id)
        return

    logging.info("Tags on %s %s: %s", TT_TO_STR[target_type], target_id, ", ".join(tag["name"] for tag in tags))


def search_tag(api_host, tag_name):
    tags = find_tags(api_host, {"name": tag_name})

    if not tags:
        logging.warning("Tag %s is not set on anything", tag_name)
        return

    logging.info("Items tagged %s:", tag_name)
    for tag in tags:
        logging.info("* %s %s", TT_TO_STR[tag["target_type"]], tag["target_id"])


def safety_check(args):
    if args["--api-host"] == "ab.yandex-team.ru" and not args["--allow-editing-production-data"]:
        raise Exception("Add --allow-editing-production-data to change data on production instance!")


def main():
    args = docopt.docopt(USAGE)
    umisc.configure_logger(verbose=int(args["--verbose"]))

    api_host = args["--api-host"]

    if args["search"]:
        search_tag(api_host, args["<tag-name>"])
    else:
        target_type = None
        target_id = None
        if args["task"]:
            target_type = TARGET_TYPE_TASK
            target_id = args["<task-id>"]
        elif args["obs"]:
            target_type = TARGET_TYPE_OBSERVATION
            target_id = args["<obs-id>"]
        elif args["exp"]:
            target_type = TARGET_TYPE_EXPERIMENT
            target_id = args["<testid>"]

        assert target_type is not None and target_id is not None

        if not args["--add"] and not args["--delete"]:
            view_tags(api_host, target_type, target_id)

        if args["--add"]:
            safety_check(args)
            for tag in args["--add"]:
                add_tag(api_host, target_type, target_id, tag)

        if args["--delete"]:
            safety_check(args)
            for tag in args["--delete"]:
                del_tag(api_host, target_type, target_id, tag)


if __name__ == "__main__":
    main()
