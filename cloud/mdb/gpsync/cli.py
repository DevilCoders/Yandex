# coding: utf-8
"""
Various utility fucntions:
    - Zookeeper structures init
    - Scheduled switchover
"""
import argparse
import functools
import socket
import sys
import logging

from . import read_config, init_logging, zk
from . import helpers
from . import utils
from .exceptions import SwitchoverException


class ParseHosts(argparse.Action):
    """
    Check validity of provided hostnames
    """

    def __call__(self, parser, namespace, values, option_string=None):
        for value in values:
            try:
                socket.getaddrinfo(value, 0)
            except Exception as exc:
                raise ValueError('invalid hostname: %s: %s' % (value, exc))
            namespace.members.append(value)


def entry():
    """
    Entry point.
    """
    opts = parse_args()
    conf = read_config(
        filename=opts.config_file,
        options=opts,
    )
    init_logging(conf)
    try:
        opts.action(opts, conf)
    except (KeyboardInterrupt, EOFError):
        logging.error('abort')
        sys.exit(1)
    except RuntimeError as err:
        logging.error(err)
        sys.exit(1)
    except Exception as exc:
        logging.exception(exc)
        sys.exit(1)


def maintenance_enabled(zookeeper):
    """
    Returns True if all hosts confirmed that maintenance is enabled.
    """
    for host in zk.get_alive_hosts(zookeeper):
        if zookeeper.get(zk.get_host_maintenance_path(host)) != 'enable':
            return False
    return True


def maintenance_disabled(zookeeper):
    """
    Common maintenance node should be deleted
    """
    return zookeeper.get(zk.MAINTENANCE_PATH) is None


def _wait_maintenance_enabled(zookeeper, timeout):
    is_maintenance_enabled = functools.partial(maintenance_enabled, zookeeper)
    if not helpers.await_for(is_maintenance_enabled, timeout, 'Fail to enable maintenance mode'):
        # Return cluster to last state, i.e. disable maintenance.
        zookeeper.write(zk.MAINTENANCE_PATH, 'disable')
        raise TimeoutError
    logging.info('Success')


def _wait_maintenance_disabled(zookeeper, timeout):
    is_maintenance_disabled = functools.partial(maintenance_disabled, zookeeper)
    if not helpers.await_for(is_maintenance_disabled, timeout, 'Fail to disable maintenance mode'):
        # Return cluster to last state, i.e. enable maintenance.
        # There is obvious race condition between time when master deletes this node
        # and we write value here. We assume that big timeout will help us here.
        zookeeper.write(zk.MAINTENANCE_PATH, 'enable')
        raise TimeoutError
    logging.info('Success')


def maintenance(opts, conf):
    """
    Enable or disable maintenance mode.
    """
    conn = zk.Zookeeper(config=conf, plugins=None)
    if opts.mode == 'enable':
        conn.ensure_path(zk.MAINTENANCE_PATH)
        conn.noexcept_write(zk.MAINTENANCE_PATH, 'enable', need_lock=False)
        if opts.wait_all:
            _wait_maintenance_enabled(conn, opts.timeout)
    elif opts.mode == 'disable':
        conn.write(zk.MAINTENANCE_PATH, 'disable', need_lock=False)
        if opts.wait_all:
            _wait_maintenance_disabled(conn, opts.timeout)
    elif opts.mode == 'show':
        val = conn.get(zk.MAINTENANCE_PATH) or 'disable'
        print('{val}d'.format(val=val))


def initzk(opts, conf):
    """
    Creates structures in zk.MEMBERS_PATH corresponding
    to members` names.
    """
    conn = zk.Zookeeper(config=conf, plugins=None)
    for host in opts.members:
        path = '{members}/{host}'.format(members=zk.MEMBERS_PATH, host=host)
        logging.debug('creating "%s"...', path)
        conn.ensure_path(path)
    logging.debug('ZK structures are initialized')


def switchover(opts, conf):
    """
    Perform planned switchover.
    """
    try:
        switch = utils.Switchover(
            conf=conf, master=opts.master, timeline=opts.timeline, new_master=opts.destination, timeout=opts.timeout
        )
        if opts.reset:
            return switch.reset(force=True)
        logging.info('switchover %(master)s (timeline: %(timeline)s) ' 'to %(sync_replica)s', switch.plan())
        # ask user confirmation if necessary.
        if not opts.yes:
            helpers.confirm()
        # perform returns False on soft-fail.
        # right now it happens when an unexpected host has become
        # the new master instead of intended sync replica.
        if not switch.is_possible():
            logging.error('Switchover is impossible now.')
            sys.exit(1)
        if not switch.perform(opts.replicas, block=opts.block):
            sys.exit(2)
    except SwitchoverException as exc:
        logging.error('unable to switchover: %s', exc)
        sys.exit(1)


def parse_args():
    """
    Parse multiple commands.
    """
    arg = argparse.ArgumentParser(
        description="""
        PGSync utility
        """
    )
    arg.add_argument(
        '-c',
        '--config',
        dest='config_file',
        type=str,
        metavar='<path>',
        default='/etc/gpsync.conf',
        help='path to gpsync main config file',
    )
    arg.add_argument(
        '--zk',
        type=str,
        dest='zk_hosts',
        metavar='<fqdn:port>,[<fqdn:port>,...]',
        help='override config zookeeper connection string',
    )
    arg.add_argument(
        '--zk-prefix',
        metavar='<path>',
        type=str,
        dest='zk_lockpath_prefix',
        help='override config zookeeper path prefix',
    )

    subarg = arg.add_subparsers(
        help='possible actions', title='subcommands', description='for more info, see <subcommand> -h'
    )

    # Init ZK command
    initzk_arg = subarg.add_parser('initzk', help='define zookeeper structures')
    initzk_arg.add_argument(
        'members',
        metavar='<fqdn> [<fqdn> ...]',
        action=ParseHosts,
        default=[],
        nargs='+',
        help='Space-separated list of cluster members hostnames',
    )
    initzk_arg.set_defaults(action=initzk)

    maintenance_arg = subarg.add_parser('maintenance', help='maintenance mode')
    maintenance_arg.add_argument(
        '-m', '--mode', metavar='[enable, disable, show]', default='enable', help='Enable or disable maintenance mode'
    )
    maintenance_arg.add_argument(
        '-w',
        '--wait_all',
        help='Wait for all alive high-availability hosts finish entering/exiting maintenance mode',
        action='store_true',
        default=False,
    )
    maintenance_arg.add_argument(
        '-t', '--timeout', help='Set timeout for maintenance command with --wait_all option', type=int, default=5 * 60
    )
    maintenance_arg.set_defaults(action=maintenance)

    # Scheduled switchover command
    switch_arg = subarg.add_parser(
        'switchover',
        help='perform graceful switchover',
        description="""
        Perform graceful switchover of the current master.
        The default is to auto-detect its hostname and
        timeline in ZK.
        This behaviour can be overridden with options below.
        """,
    )
    switch_arg.add_argument('-d', '--destination', help='sets host where to switch', default=None, metavar='<fqdn>')
    switch_arg.add_argument(
        '-b', '--block', help='block until switchover completes or fails', default=False, action='store_true'
    )
    switch_arg.add_argument(
        '-t',
        '--timeout',
        help='limit each step to this amount of seconds',
        type=int,
        default=60,
        metavar='<sec>',
    )
    switch_arg.add_argument(
        '-y', '--yes', help='do not ask confirmation before proceeding', default=False, action='store_true'
    )
    switch_arg.add_argument(
        '-r',
        '--reset',
        help='reset switchover state in ZK (potentially disruptive)',
        default=False,
        action='store_true',
    )
    switch_arg.add_argument(
        '--replicas',
        help='if in blocking mode, wait until this number of replicas become' ' online',
        type=int,
        default=2,
        metavar='<int>',
    )
    switch_arg.add_argument('--master', help='override current master hostname', default=None, metavar='<fqdn>')
    switch_arg.add_argument('--timeline', help='override current master timeline', default=None, metavar='<fqdn>')
    switch_arg.set_defaults(action=switchover)

    try:
        return arg.parse_args()
    except ValueError as err:
        arg.exit(message='%s\n' % err)
        exit(1)
