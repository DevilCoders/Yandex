#!/usr/bin/env python

import logging

from tornado import gen
from tornado import ioloop
from cocaine.tools.actions import group
from cocaine.services import Service

logger = logging.getLogger("cocaine.admin")
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


ENDPOINTS = [("localhost", 10071)]

@gen.coroutine
def get_app_list():
    logger.debug("fetching an application list")
    #get app from runlist
    node = Service("node", endpoints=ENDPOINTS)
    chan = yield node.list()
    app_runlist = yield chan.rx.get()
    #get not null weight app from active groups
    storage = Service("storage", endpoints=ENDPOINTS)
    group_list = yield group.List(storage).execute()
    app_group_list = []
    for group_name in group_list:
        group_apps = yield group.View(storage,group_name).execute()
        for app in group_apps:
            if int(group_apps[app]) != 0:
                app_group_list.append(app)
    app_list = []
    #intersection two list
    if app_group_list == []:
        app_list = app_runlist
    else:
        for app_group in app_group_list:
            if app_group in app_runlist:
                if app_group not in app_list:
                    app_list.append(app_group)
    logger.debug("there are %d apps", len(app_list))
    raise gen.Return(app_list)

@gen.coroutine
def spawn_worker(name, node):
    logger.debug("spawning a worker for %s", name)
    try:
        chan = yield node.info(name,0x04|0x01)
        node_info = yield chan.rx.get()
        try:
            concurrency = node_info["profile"]["data"]["concurrency"]
        except:
            concurrency = 10
        app = Service(name, endpoints=ENDPOINTS)
        chan = yield app.control()
        if concurrency == 1 and node_info["pool"]["capacity"] > 1:
            yield chan.tx.write(2)
        else:
            yield chan.tx.write(1)
        yield gen.sleep(1)
        chan = yield app.info()
        info = yield chan.rx.get()
        if len(info["pool"]["slaves"]) >= 1:
            logger.info("%s has been started", name)
    except Exception as err:
        logger.error("unable to spawn a %s: %s", name, err)
        raise gen.Return(str(err))
    raise gen.Return("OK")

@gen.coroutine
def spawn_workers(apps):
    node = Service("node", endpoints=ENDPOINTS)
    for name in apps:
        res = yield spawn_worker(name, node)
        logger.info("%s: %s", name, res)
        yield gen.sleep(0.1)

@gen.coroutine
def main():
    while True:
        try:
            apps = yield get_app_list()
        except Exception:
            yield gen.sleep(1)
        else:
            break
    yield spawn_workers(apps)

if __name__ == "__main__":
    ioloop.IOLoop.current().run_sync(main, timeout=300)

