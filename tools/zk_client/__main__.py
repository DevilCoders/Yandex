import sys
import argparse
import os

from kazoo.client import KazooClient, KazooState


def create_client(hosts):
    def lost_connection():
        print >>sys.stderr, 'connection lost'
        os._exit(100)

    def listener(state):
        if state == KazooState.LOST:
            lost_connection()

    zk = KazooClient(hosts=','.join([x + ':2181' for x in hosts]), randomize_hosts=True)

    zk.start(timeout=60)
    zk.add_listener(listener)

    return zk


def do_get(args):
    parser = argparse.ArgumentParser()

    parser.add_argument('-z', '--zk-host', help='ZooKeeper host', required=True)
    parser.add_argument('-p', '--path', help='Path to list', required=True)

    args = parser.parse_args(args)

    res = create_client([args.zk_host]).get(args.path)

    print res[0]
    print res[1]


def do_exists(args):
    parser = argparse.ArgumentParser()

    parser.add_argument('-z', '--zk-host', help='ZooKeeper host', required=True)
    parser.add_argument('-p', '--path', help='Path to list', required=True)

    args = parser.parse_args(args)

    print create_client([args.zk_host]).exists(args.path)


def do_list(args):
    parser = argparse.ArgumentParser()

    parser.add_argument('-z', '--zk-host', help='ZooKeeper host', required=True)
    parser.add_argument('-p', '--path', help='Path to list', required=True)
    parser.add_argument('-r', '--recursive', help='List recursively', action='store_true')

    args = parser.parse_args(args)

    zk = create_client([args.zk_host])

    def iter_one(p):
        for x in zk.get_children(p):
            yield p + '/' + x

    def iter_recurse(p):
        v = set()

        def do_iter(n):
            if n not in v:
                yield n

                v.add(n)

                for x in iter_one(n):
                    for y in do_iter(x):
                        yield y

        return do_iter(p)

    if args.recursive:
        func = iter_recurse
    else:
        func = iter_one

    for x in func(args.path):
        print x


def do_delete(args):
    parser = argparse.ArgumentParser()

    parser.add_argument('-z', '--zk-host', help='ZooKeeper host', required=True)
    parser.add_argument('-p', '--path', help='Path to delete', required=True)
    parser.add_argument('-r', '--recursive', help='Delete recursively', action='store_true')

    args = parser.parse_args(args)

    create_client([args.zk_host]).delete(args.path, recursive=args.recursive)


def do_command(args):
    parser = argparse.ArgumentParser()

    parser.add_argument('-z', '--zk-host', help='ZooKeeper host', required=True)
    parser.add_argument('command')

    args = parser.parse_args(args)

    print create_client([args.zk_host]).command(args.command)


def main():
    if len(sys.argv) < 2:
        print 'use one of %s [list, get, delete] --help' % sys.argv[0]

        return

    if sys.argv[1] == 'list':
        do_list(sys.argv[2:])
    elif sys.argv[1] == 'get':
        do_get(sys.argv[2:])
    elif sys.argv[1] == 'delete':
        do_delete(sys.argv[2:])
    elif sys.argv[1] == 'command':
        do_command(sys.argv[2:])
    elif sys.argv[1] == 'exists':
        do_exists(sys.argv[2:])
    else:
        raise Exception('unsupported mode %s' % sys.argv[1])


if __name__ == '__main__':
    try:
        main()
    except IOError:
        pass  # Broken pipe is ok
