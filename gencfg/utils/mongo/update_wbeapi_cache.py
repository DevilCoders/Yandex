#!/skynet/python/bin/python
"""
    This script represents api of caching layer of gencfg api/wbe. In this script we can get cached gencfg response from (<wbe|api>, <commit>, <request>)
    and update this cache as well
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import zlib
import pymongo
import bson
import multiprocessing
import time

import gencfg
import flask
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from gaux.aux_mongo import get_mongo_collection
from core.settings import SETTINGS
from core.db import CURDB, set_current_thread_CURDB, get_real_db_object
from gaux.aux_utils import print_progress
import ujson


class EActions(object):
    SHOW = "show"
    SHOWCACHED = "showcached"
    UPDATE = "update"
    UPDATEALL = "updateall"
    ALL = [SHOW, SHOWCACHED, UPDATE, UPDATEALL]


class EBackendTypes(object):
    WBE = "wbe"
    API = "api"
    ALL = [WBE, API]

FLASK_API = None

def get_parser():
    parser = ArgumentParserExt("Script to show/update cache data from gencfg")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-b", "--backend-type", type=str, default=None,
                        choices=EBackendTypes.ALL,
                        help="Optional. Backend type (obligatory for <%s> and <%s> actions)" % (
                        EActions.SHOW, EActions.UPDATE))
    parser.add_argument("-c", "--commit", type=int, default=None,
                        help="Optional. Commit id (obligatory for <%s> and <%s> actions)" % (
                        EActions.SHOW, EActions.UPDATE))
    parser.add_argument("-r", "--request", type=str, default=None,
                        help="Optional. Request to cache (obligatory for <%s> and <%s> actions)" % (
                        EActions.SHOW, EActions.UPDATE))
    parser.add_argument("-e", "--response", type=str, default=None,
                        help="Optional. Argument for action <%s>: response body" % EActions.UPDATE)
    parser.add_argument("--quiet", action="store_true", default=False,
                        help="Optional. Do not print excessive messages")
    parser.add_argument("-w", "--workers", type=int, default=6,
                        help="Optional. Number of parallel workers to calculate cache (use 1 worker to disable multiprocessing)")
    parser.add_argument("--remove-old", action="store_true", default=False,
                        help="Optional. Remove old data from cache")
    parser.add_argument('--dry-run', action='store_true', default=False,
                        help='Optional. Run as dry run (do not save to mongo)')

    return parser


def normalize(options):
    pass


def generate_backend_client(backend_type):
    """
        Create testing flask application, which can be used to process gencfg backend/api request without raising backend
    """
    # prepare application
    jsonargs = {"db": "unstable", "disable_log": True, "allow_auth": False}
    if backend_type == EBackendTypes.WBE:
        from wbe_backend.main import prepare_wbe
        app, _ = prepare_wbe(jsonargs=jsonargs)
    elif backend_type == EBackendTypes.API:
        from api_backend.main import prepare_api
        app, _ = prepare_api(jsonargs=jsonargs)
    else:
        raise Exception("Unknown backend type <%s>" % backend_type)

    app.testing = True
    test_client = app.test_client()

    return test_client


def update_mongo_record(mongocoll, backend_type, backend_version, commit, request, response):
    """
        Update record with specified key (<backend_type>, <backend_version>, <commit>, <request>) in mongodb

        :type mongocoll: pymongo.collectcion
        :type backend_type: str
        :type backend_version: str
        :type commit: int
        :type request: str
        :type response: str

        :param mongocoll: mongo collection to insert record in
        :param backend_type: backend type (<wbe> or <api> currently)
        :param backend_version: backend version (when we update code, we should increase version in settings/settings.yaml)
        :param commit: commit id
        :param requst: request path to wbe/api, e. g. </searcherlookup/groups/MSK_RESERVED/instances>
        :param response: response content as text string

        :return None:
    """
    # than insert new data
    mongocoll.insert({
        "backend_type": backend_type,
        "backend_version": backend_version,
        "commit": commit,
        "request": request,
        "response": bson.binary.Binary(response),
    })

class CalculateSingleReqCache(object):
    def __init__(self, backend_type):
        self.backend_type = backend_type
        if backend_type == EBackendTypes.WBE:
            self.backend_version = SETTINGS.backend.wbe.version
        elif backend_type == EBackendTypes.API:
            self.backend_version = SETTINGS.backend.api.version
        self.last_commit = CURDB.get_repo().get_last_commit().commit

    def __call__(self, url):
        global FLASK_API

        response = FLASK_API.get("/unstable%s" % url)
        if response._status_code != 200:
            raise Exception("Got status <%s> when processing request <%s>" % (response._status_code, "/unstable%s" % url))

        jsoned = ujson.loads(response.data)
        if "debug_info" in jsoned:
            jsoned.pop("debug_info")
        response = zlib.compress(ujson.dumps(jsoned))

        return {
            "backend_type": self.backend_type,
            "backend_version": self.backend_version,
            "commit": self.last_commit,
            "request": url,
            "response": bson.binary.Binary(response),
        }

def caclulate_wbeapi_cache(options, backend_type, items_descr, items_data, request_str_func):
    """
        In this function array with cache data for specified <items_data> object is filled. Request to backend is constructed via <request_str_func>
    """


    print "        Collecting data for <%s> (%d items):" % (items_descr, len(items_data))

    global FLASK_API
    FLASK_API = generate_backend_client(backend_type)

    caller = CalculateSingleReqCache(backend_type)

    if options.workers > 1:
        pool = multiprocessing.Pool(options.workers)
        urls = map(request_str_func, items_data)
        result = pool.map(caller, urls)
    else:
        result = []
        for i, item in enumerate(items_data):
            if not options.quiet:
                print_progress(i, len(items_data))
            url = request_str_func(item)
            result.append(caller(url))
        if not options.quiet:
            print_progress(len(items_data), len(items_data), stopped=True)

    return result


def main(options):
    mongocoll = get_mongo_collection('gencfgapiwbecache')

    if options.backend_type == EBackendTypes.WBE:
        backend_version = SETTINGS.backend.wbe.version
    elif options.backend_type == EBackendTypes.API:
        backend_version = SETTINGS.backend.api.version

    if options.action == EActions.SHOWCACHED:
        response = mongocoll.find_one({
            "backend_type": options.backend_type,
            "backend_version": backend_version,
            "commit": options.commit,
            "request": options.request,
        })

        if response is not None:
            response = response["response"]
            response = zlib.decompress(response)

        return response
    elif options.action in [EActions.UPDATE, EActions.SHOW]:
        if options.response is None:
            response = generate_backend_client(options.backend_type).get("/unstable%s" % options.request).data
            jsoned = ujson.loads(response)
            if "debug_info" in jsoned:
                jsoned.pop("debug_info")
            response = ujson.dumps(jsoned)
        else:
            response = options.response

        if options.action == EActions.UPDATE:
            response = zlib.compress(response)
            update_mongo_record(mongocoll, options.backend_type, backend_version, options.commit, options.request, response)
        elif options.action == EActions.SHOW:
            print "Request: <%s>\nResponse: <%s>" % (options.request, response)
        else:
            raise Exception("Unknown action <%s>" % options.action)

    elif options.action == EActions.UPDATEALL:  # update cache for all requests to specified commit
        groups_to_process = [x for x in CURDB.groups.get_groups() if x.card.properties.created_from_portovm_group is None]
        DATA = [
            # cache for api
            (EBackendTypes.API, "Group cards", groups_to_process, lambda group: "/groups/%s/card?" % group.card.name),
            (EBackendTypes.API, "Ctypes", CURDB.ctypes.get_ctypes(), lambda ctype: "/ctypes/%s?" % ctype.name),
            (EBackendTypes.API, "Group instances", groups_to_process, lambda group: "/groups/%s/instances?" % group.card.name),
            (EBackendTypes.API, "Group searcherlookups", groups_to_process, lambda group: "/searcherlookup/groups/%s/instances?" % group.card.name),
            (EBackendTypes.API, "Hosts instances tags", CURDB.groups.get_group('ALL_SEARCH').getHosts(), lambda host: "/hosts/%s/instances_tags?" % host.name),
            # cache for wbe
            (EBackendTypes.WBE, "Wbe main pages", ["/groups?", "/tiers?", "/ctypes?", "/itypes?", "/intlookups?", "/hosts?"], lambda elem: elem),
        ]
        mongo_insert_data = []
        last_commit = CURDB.get_repo().get_last_commit().commit

        print "Updating cache for commit <%s>:" % last_commit

        print "    Collecting responses:"
        for item in DATA:
            startt = time.time()
            mongo_insert_data.extend(caclulate_wbeapi_cache(options, *item))
            set_current_thread_CURDB(get_real_db_object())
            endt = time.time()
            print '            Section <{}> loaded in <{:.2f}> seconds'.format(item[1], endt - startt)

        if not options.dry_run:
            if options.remove_old:
                print "    Cleaning old cache:"
                mongocoll.remove({
                    "commit": last_commit,
                })

            print "    Inserting bulk data:"
            bulk = mongocoll.initialize_unordered_bulk_op()
            for elem in mongo_insert_data:
                bulk.insert(elem)
            bulk.execute()
    else:
        raise Exception("Unknown action <%s>" % options.action)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


def print_result(result, options):
    if options.action == EActions.SHOWCACHED:
        if result is None:
            result = "Not cached"
        print "Request <%s> to backend <%s> from commit <%s>: <%s>" % (
        options.request, options.backend_type, options.commit, result)
    elif options.action == EActions.SHOW:
        pass
    elif options.action == EActions.UPDATE:
        pass
    elif options.action == EActions.UPDATEALL:
        pass
    else:
        raise Exception("Unknown action <%s>" % options.action)


if __name__ == "__main__":
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
