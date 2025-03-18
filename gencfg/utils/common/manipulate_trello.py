#!/skynet/python/bin/python
"""
    Script to manipulate with trello work board (and sync it with startrek). Can perform following actions:
       - create trello task based on sandbox task.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import urllib2
import json
import re

import config
import gencfg
import requests
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
import core.argparse.types

class EActions(object):
    CREATETASK = "createtask"
    ALL = [CREATETASK]


def get_parser():
    parser = ArgumentParserExt(description="Perform various actions with sandbox schedulers")
    parser.add_argument("-a", "--action", type=str, default = EActions.CREATETASK,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-s", "--startrek-task", type = str, default = None,
                        help = "Mame of startrek task (for actions <%s>)" % EActions.CREATETASK)
    parser.add_argument("-l", "--trello-list", type = str, default = "Current",
                        help = "Name of trello list to add (for actions <%s>)" % EActions.CREATETASK)
    parser.add_argument("-w", "--watchers", type = core.argparse.types.comma_list, default = [],
                        help = "List of extra watchers for trello task (for actions <%s>)" % EActions.CREATETASK)
    parser.add_argument("--add-on-top", action="store_true", default=False,
                        help = "Optional. Put created card at top of the list")

    return parser

def normalize(options):
    if options.action == EActions.CREATETASK:
        if options.startrek_task is None:
            raise Exception("Options <--startrek-task> must be specified with action <%s>" % options.action)

        if options.startrek_task.find('/') > 0:
            options.startrek_task = options.startrek_task.rpartition('/')[2]

def _trello_prefix(path):
    return "%s/%s?key=%s&token=%s" % (SETTINGS.services.trello.rest.url, path, SETTINGS.services.trello.rest.application_key, SETTINGS.services.trello.rest.token)


def get_trello_list_id(options):
    # first find id of trello list to add task to
    trello_board_url = "%s&lists=open" % _trello_prefix("/boards/%s" % SETTINGS.services.trello.rest.board)
    trello_board_json = json.loads(urllib2.urlopen(trello_board_url).read())

    trello_list_id = filter(lambda x: x["name"] == options.trello_list, trello_board_json["lists"])
    if len(trello_list_id) == 0:
        raise Exception("Not found trello list <%s>" % options.trello_list)
    trello_list_id = trello_list_id[0]["id"]

    return trello_list_id


def get_startrek_task(options):
    task_url = "%sv2/issues/%s" % (SETTINGS.services.startrek.rest.url, options.startrek_task)
    headers = {"Authorization": "OAuth %s" % config.get_default_oauth()}
    req = urllib2.Request(task_url, None, headers)

    startrek_task_json = json.loads(urllib2.urlopen(req).read())

    return startrek_task_json


def create_trello_card(options, trello_list_id, created_by, description):
    # create card
    watchers = " ".join(map(lambda x:  "(%s)" % x, [created_by] + options.watchers))

    trello_card_name = "%s %s" % (watchers.encode("utf8"), description.encode("utf8"))
    trello_card_desc = "%s%s" % (SETTINGS.services.startrek.http.url, options.startrek_task)
    trello_create_card_url = "%s&name=%s&desc=%s" % (_trello_prefix("/lists/%s/cards" % trello_list_id), urllib2.quote(trello_card_name), urllib2.quote(trello_card_desc))

    trello_card_json = json.loads(urllib2.urlopen(trello_create_card_url, "").read())

    trello_card_id = trello_card_json["id"]
    trello_card_url = trello_card_json["url"]

    # move to top if specified in options
    if options.add_on_top:
        trello_move_to_top_url = "%s&value=top" % _trello_prefix("/cards/%s/pos" % trello_card_id)
        resp = requests.put(trello_move_to_top_url, data="")
        if resp.status_code != 200:
            raise Exception, "Put request for url <%s> failed" % trello_move_to_top_url

    return trello_card_id, trello_card_url


def set_trello_card_label(options, trello_card_id, label_name, label_color = None):
    del options
    url = "%s&name=%s" % (_trello_prefix("/cards/%s/labels" % trello_card_id), urllib2.quote(label_name))
    if label_color is not None:
        url = "%s&color=%s" % (url, label_color)

    urllib2.urlopen(url, "").read()

def main(options):
    if options.action == EActions.CREATETASK:
        trello_list_id = get_trello_list_id(options)

        startrek_task_json = get_startrek_task(options)

        trello_card_id, trello_card_url = create_trello_card(options, trello_list_id, startrek_task_json["createdBy"]["id"], startrek_task_json["summary"])

        set_trello_card_label(options, trello_card_id, startrek_task_json["createdBy"]["id"], None)
        for watcher in options.watchers:
            set_trello_card_label(options, trello_card_id, watcher, None)

        print "Created trello card <%s>" % trello_card_url
    else:
        raise Exception("Unknown action <%s>" % options.action)

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
