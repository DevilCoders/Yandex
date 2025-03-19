{%- from "components/greenplum/map.jinja" import gpdbvars with context -%}
# !/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import psycopg2
import argparse
from subprocess import Popen, PIPE, STDOUT
from enum import Enum
from os import path
import sys

GP_HOME = '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
LOG_FILE = '{{ gpdbvars.gplog }}/gp_autorecovery.log'
AUTORECOVERY_DISABLED_FLAG = '/tmp/.autorecovery_disabled.flag'

SELECT_DEAD_SEGMENTS = "select hostname from pg_catalog.gp_segment_configuration where status='d'"
SELECT_SEGMENTS_INFO = "select content, role, preferred_role from pg_catalog.gp_segment_configuration where status='u'"


class Status(Enum):
    OK = 1
    ALL_VMS_DEAD = 2
    ALL_ALIVE_AFTER_RECOVERY = 3
    SOMETHING_GOES_WRONG = 4
    CAN_NOT_CONNECT = 5


def is_rebalancing_required():
    log = logging.getLogger('CHECK IF REBALANCING REQUIRED')
    conn = psycopg2.connect('user={{ gpdbvars.gpadmin }} dbname=postgres')
    cur = conn.cursor()
    cur.execute(SELECT_SEGMENTS_INFO)
    info = [i for i in cur.fetchall()]
    content_to_rebalance = set()
    for line in info:
        content, role, preferred_role = line
        if content in content_to_rebalance:
            log.debug("Content {0} requires rebalancing".format(content))
            return True
        if role != preferred_role:
            content_to_rebalance.add(content)
    return False


def get_dead_segments():
    try:
        log = logging.getLogger('SELECT DEAD SEGMENTS')
        conn = psycopg2.connect('user={{ gpdbvars.gpadmin }} dbname=postgres')
        cur = conn.cursor()
        cur.execute(SELECT_DEAD_SEGMENTS)
    except psycopg2.OperationalError:
        return False, []
    dead_segments = set([i[0] for i in cur.fetchall()])
    if len(dead_segments) == 0:
        log.info('There is no dead segments, nothing to do')
    else:
        log.warning('Found some dead segments: {0}'.format(dead_segments))
    return True, dead_segments


def check_vm_alive(fqdn):
    log = logging.getLogger('CHECK {0} IS ALIVE'.format(fqdn))
    process = Popen(['ssh', fqdn, 'echo -n ok'], stdout=PIPE, stderr=PIPE)
    stdout, stderr = process.communicate()
    ok = stderr == '' and stdout == 'ok'
    if ok:
        log.info('{0} looks alive'.format(fqdn))
    else:
        log.warning('{0} looks dead'.format(fqdn))
        log.debug("'ok' expected in stdout, but '{0}'".format(stdout))
        log.debug("command stderr: '{0}'".format(stderr))

    return ok


def run_gprecoverseg(gphome, do_not_prompt=True, full_recovery=False, rebalance=False):
    command = 'source {0}/greenplum_path.sh && gprecoverseg'.format(gphome)
    if do_not_prompt:
        command += ' -a'
    if full_recovery:
        command += ' -F'
    if rebalance:
        command += ' -r'
    log = logging.getLogger('RUN {0}'.format(command))
    log.info(command)
    process = Popen(command, shell=True, stdout=PIPE, stderr=STDOUT)
    with process.stdout:
        for line in iter(process.stdout.readline, b''):
            log.debug(line)
    ok = process.wait()
    if ok == 0:
        log.info("Runs successfully")
    else:
        log.error("Some problems with gprecoverseg. See debug above")
    return ok


def ensure_segments_up(gphome):
    log = logging.getLogger('ENSURE SEGMENTS UP')
    log.info('Try to find dead segments')
    ok, dead_segments = get_dead_segments()
    if not ok:
        return Status.CAN_NOT_CONNECT
    if len(dead_segments) == 0:
        return Status.OK
    log.warning('Segments {0} dead'.format(', '.join(list(dead_segments))))

    alive_vms = set()
    some_vm_alive = False
    log.info("Check if vm's alive")
    for segment in dead_segments:
        vm_alive = check_vm_alive(segment)
        if vm_alive:
            alive_vms.add(segment)
            some_vm_alive = True
    if not some_vm_alive:
        log.info("There is no alive vm's with dead segments. Nothing to do")
        return Status.ALL_VMS_DEAD

    log.debug("Alive vm's : {0}. Try to recover".format(', '.join(list(alive_vms))))
    ok = run_gprecoverseg(gphome)

    log.info("Try to find dead segments in alive vm's")
    _, dead_segments = get_dead_segments()
    intersection = dead_segments & alive_vms
    if len(intersection) == 0:
        log.info("There is no one")
        return Status.ALL_ALIVE_AFTER_RECOVERY

    log.info("Found {0}".format(', '.join(list(intersection))))
    log.info("Try to recover with full_recovery(-F)")
    ok = run_gprecoverseg(gphome, full_recovery=True)

    _, dead_segments = get_dead_segments()
    log.info("Dead segments on alive vm's {0}".format(', '.join(list(dead_segments & alive_vms))))
    log.info("Dead vm's {0}".format(', '.join(list(dead_segments - alive_vms))))
    if len(dead_segments & alive_vms) == 0:
        return Status.ALL_ALIVE_AFTER_RECOVERY
    return Status.SOMETHING_GOES_WRONG


if __name__ == '__main__':
    arg = argparse.ArgumentParser(
        description="""
             Greenplum auto recovery (runs gprecoverseg with different args after some checks)
             """
    )
    arg.add_argument(
        '--gphome',
        dest='gphome',
        default='/opt/greenplum-db-6',
        help='GP home directory',
    )
    arg.add_argument(
        '-t',
        '--tries',
        dest='tries',
        default=1,
        help='Number of tries to recover segments',
    )
    arg.add_argument('-r', '--rebalance', action='store_const', const=True, help='Allow segment rebalancing')
    args = arg.parse_args()

    logging.basicConfig(
        filename=LOG_FILE, filemode='a', level='DEBUG', format='%(asctime)s [%(levelname)s] %(name)s:\t%(message)s'
    )

    log = logging.getLogger('MAIN')
    if path.exists(AUTORECOVERY_DISABLED_FLAG):
        log.info("Autorecovery is disabled (flag found)")
        sys.exit(0)

    for i in range(int(args.tries)):
        log.info("Try {0}".format(i + 1))
        status = ensure_segments_up(args.gphome)
        if status in [Status.OK, Status.ALL_ALIVE_AFTER_RECOVERY]:
            log.info("All segments on alive vm's is up")
        elif status == Status.ALL_VMS_DEAD:
            log.warning("All dead segments on dead vm's")
        elif status == Status.CAN_NOT_CONNECT:
            log.warning("Can't connect to db. Exiting")
            break
        else:
            log.warning("Can't recovery dead segments on alive vm's")
        if args.rebalance and is_rebalancing_required():
            log.info("Try to rebalance")
            ok = run_gprecoverseg(args.gphome, rebalance=True)
