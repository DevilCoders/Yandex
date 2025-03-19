#!/usr/bin/env python2.7
import MySQLdb
import MySQLdb.cursors
import yaml
import sys


CONFIG_PATH = "/etc/yandex/sphinx-monitoring/config.yaml"


def read_config():
    """ Read monitoring config """
    with open(CONFIG_PATH) as config_fd:
        return yaml.load(config_fd)


def get_show_status():
    """ Read show status from searchd daemon """
    con = MySQLdb.connect(
        host=read_config()["host"],
        port=read_config()["port"],
        cursorclass=MySQLdb.cursors.DictCursor
    )
    cur = con.cursor()
    cur.execute("SHOW STATUS")
    for item in cur.fetchall():
        yield item["Counter"], item["Value"]


def mode_graphite():
    """ Graphite mode worker """
    data = {}
    for item in get_show_status():
        try:
            key = item[0]
            val = float(item[1])
        except ValueError:
            continue
        data.update({key: val})
    for key, val in iter(data.items()):
        print("%s %s" % (key, val))


def mode_juggler():
    """ Reserved for future implementations """
    raise NotImplemented


def main():
    """ Main routine """
    if "--graphite" in sys.argv:
        mode_graphite()
    if "--juggler" in sys.argv:
        mode_juggler()


if __name__ == "__main__":
    main()
