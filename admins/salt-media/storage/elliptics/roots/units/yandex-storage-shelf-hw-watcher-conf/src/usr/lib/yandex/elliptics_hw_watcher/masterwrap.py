#!/usr/bin/pymds
# -*- coding: utf-8 -*-

import sys
import logging as l
import random
import socket
import os
import subprocess
import time
from mds.admin.library.python.sa_scripts import utils
from mds.admin.library.python.sa_scripts import mm
from requests.exceptions import ConnectTimeout, ReadTimeout

_format = l.Formatter("[%(asctime)s] %(levelname)s: %(message)s")
_handler = l.handlers.RotatingFileHandler(
    '/var/log/hw_watcher/elliptics.log', maxBytes=300 * 1024 * 1024, backupCount=10)
_handler.setFormatter(_format)
l.getLogger().setLevel(l.DEBUG)
l.getLogger().addHandler(_handler)

try:
    HOSTNAME = socket.gethostname()
except Exception:
    l.critical("Failed to resolve local hostname")
    sys.exit(3)


def send_email(message_subject, message_body, duty=False):
    import smtplib
    message_from = "Masterwrap <root@%s>" % HOSTNAME
    message_to = ["mds-cc@yandex-team.ru"]
    if duty:
        message_to.append("mds-duty@yandex-team.ru")
    message = "From: %s\nTo: %s\nContent-type: text/plain\nSubject: %s\n%s" % (
        message_from, ', '.join(message_to), message_subject, message_body)
    try:
        smtpObj = smtplib.SMTP('localhost')
        smtpObj.sendmail(message_from, message_to, message)
    except:
        l.error("Could not send email to %s" % message_to)


@mm.mm_retry()
def restore(mm_client, path):
    params = {
        'src_host': HOSTNAME,
        'path': path,
        'force': True,
        'autoapprove': True,
        'skip_mandatory_dcs_check': True
    }

    job_data = mm_client.request("restore_groups_from_path", params)
    l.debug("path: {}, result: {}".format(path, job_data))

    code = 0
    if 'Error' in job_data:
        return 5

    if job_data['cancelled_jobs']:
        mail_clear(path, job_data['cancelled_jobs'])
        code = 101

    if job_data['help']:
        # mail_help(path, job_data['help'])
        # Смотрим в мониторинг упавших джобов
        code = 101

    if job_data['failed']:
        for group, info in job_data['failed'].iteritems():
            # Смотрим в мониторинг упавших джобов
            if 'pending' in info:
                code = 101
            # Ошибки с монгой можно ретраить
            elif 'mongo' in info or 'waiting for replication timed out' in info:
                code = 101
            # Ошибки запросов в коллектор (чаще всего из-за inventory)
            elif 'Failed to update groups statuses' in info:
                code = 101
            # Cannot stop job 3478c781c56b4f28be971b6f80cce0e9, type is recover_dc_job and has 3 FIX_EBLOBS tasks
            # Рестор не может стопнуть recover_dc c eblob_kit, нужно руками разбирать
            # Ждём завершения конвертаций
            elif 'Cannot stop job' in info and 'type is recover_dc_job and has' not in info:
                code = 101
            # MDS-13260
            elif 'Failed to find backend by group id' in info:
                code = 101
            # MDS-13843
            elif 'Job ' in info and ' is not found' in info:
                code = 101
            else:
                return 4

    if job_data['jobs']:
        code = 101

    if code == 0:
        check, code = check_path(mm_client, path)
    return code


@mm.mm_retry()
def remove_future_backend_records_on_fs(mm_client, path):
    params = {
        'hostname': HOSTNAME,
        'fs_path': path,
        'remover': 'masterwrap',
    }

    remove_result = mm_client.request("remove_future_backend_records_on_fs", params)
    l.debug("Remove future backends result: {}".format(remove_result))

    sys.exit(0)


def mail_clear(path, jobs):
    email_message_subject = "hw-watcher: Увожу данные с диска {0} на хосте {1}".format(
        path, HOSTNAME)
    email_message_body = "Тут упали джобы и пришлось их отменить: {}\n" \
        "Вынесите мусор)".format(
            jobs)
    send_email(email_message_subject, email_message_body, duty=True)


def mail_help(path, jobs):
    email_message_subject = "hw-watcher: Увожу данные с диска {0} на хосте {1}".format(
        path, HOSTNAME)
    email_message_body = "Тут упали джобы, но отменять их почему-то нельзя: {}\n" \
        "ПОМОГИТЕ!!!!\nИ не забудьте watcher disk reset_status".format(
            jobs)
    send_email(email_message_subject, email_message_body, duty=True)


def check_file(check_file, request_id):
    if not os.path.isfile(check_file):
        return False
    f = open(check_file, 'r')
    i = f.read().split('\n')[0]
    f.close()
    if i != 'mds-autoadmin':
        if i not in request_id and request_id not in i:
            l.error("Request id in file {} mismatch. {} != {}".format(check_file, i, request_id))
            sys.exit(12)
    return True


def create_file(create_file, request_id):
    f = open(create_file, 'w')
    f.write(str(request_id))
    f.close()


def restore_path(mm_hosts, path, request_id, randint=0):
    mail_file = "/var/tmp/hw-watcher-restore-{0}-mail".format(
        path.replace('/', '-'))
    exit_code = 0

    if check_file(mail_file, request_id):
        # hw-watcher запускается раз в 10 минут, что для нас слишком часто.
        # Ручка restore_path достаточно тяжелая и, при большом количестве восстановлений
        # (https://st.yandex-team.ru/MDS-8210#5fda220bb806202d368384a5), забиваются очереди
        # Поэтому решил добавить элемент рандома, чтобы запускать каждый шестой запуск
        # (если это не первый запуск restore_path)
        if random.randint(0, randint) == 0:
            l.info(mm_hosts)
            mm_client = mm.mastermind_service_client(mm_hosts)
            exit_code = restore(mm_client, path)
        else:
            l.debug("Skip restore_path: Too Many Requests")
            exit_code = 101
    else:
        create_file(mail_file, request_id)
        l.info(mm_hosts)
        mm_client = mm.mastermind_service_client(mm_hosts)
        exit_code = restore(mm_client, path)

    if exit_code == 0:
        os.remove(mail_file)
    sys.exit(exit_code)


def check_path(mm_client, path):
    out = ''
    code = 0
    commandString = '/usr/bin/elliptics-node-info.py |grep {}'.format(path.replace('*/', ''))
    out += '{}\n'.format(commandString)
    info = subprocess.Popen(commandString, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    s_out = info.stdout.read()
    s_err = info.stderr.read()
    if s_out or s_err:
        code = 22
    out += 'stdout:\n{}\n'.format(s_out)
    out += 'stderr:\n{}\n'.format(s_err)
    o, p, c = search_by_path(mm_client, path)
    if len(o) > 0 or c != 0:
        code = 22
    out += 'search-by-path {}\n'.format(p)
    out += '{}\n'.format(o)
    return out, code


def search_by_path(mm_client, path):
    params = {
        'host': HOSTNAME,
        'path': path,
        'last': True
    }

    job_data = mm_client.request("search_history_by_path", [params])
    l.debug("path: {}, result: {}".format(path, job_data))

    code = 0
    if 'Error' in job_data:
        code = 101

    return job_data, params, code


def detach_node(mm_hosts, path, group):
    l.info(mm_hosts)
    mm_client = mm.mastermind_service_client(mm_hosts)

    res, _, code = search_by_path(mm_client, path)
    if not isinstance(res, list) or len(res) != 1 or len(res[0]['set']) != 1:
        # error
        l.critical("Invalid result from MM search-by-path: {0}".format(res))
        sys.exit(10)

    mm_group = res[0]['group']
    backend = res[0]['set'][0]['backend_id']
    if mm_group != group:
        l.critical("MM thiks that group in {0} must be {1}, not {2}".format(path, mm_group, group))
        sys.exit(11)

    node = "{host}:1025/{backend}".format(host=HOSTNAME, backend=backend)
    l.info("node {0} detach from group {1}".format(node, group))
    res = mm_client.request("group_detach_node", [group, node])
    l.debug("result: {}".format(res))
    if not res:
        sys.exit(12)


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-r", "--restore_path", help="[ %default ]", default='')
    parser.add_option("-i", "--request_id", help="[ %default ]", default=None)
    parser.add_option("-c", "--check_path", help="[ %default ]", default='')

    parser.add_option("-b", "--remove_future_backend", help="[ %default ]", default='')

    parser.add_option("-d", "--detach_path", help="[ %default ]", default='')
    parser.add_option("-g", "--detach_group", help="[ %default ]", default='')
    parser.add_option("-e", "--run_randint", help="[ %default ]", default=0, type=int)

    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


if __name__ == "__main__":
    options = parseArgs(sys.argv)
    try:
        attempts = 5
        for attempt in xrange(1, attempts + 1):
            try:
                mm_hosts = utils.get_mastermind_hosts()
            except (ConnectTimeout, ReadTimeout):
                if attempt == attempts:
                    sys.exit(101)
                else:
                    time.sleep(30)
                    continue
            else:
                break

        if options.restore_path:
            if options.request_id is None:
                l.error("Reqiest id is None")
                sys.exit(33)
            else:
                restore_path(mm_hosts, os.path.normpath(options.restore_path) + '/', options.request_id, options.run_randint)
        if options.remove_future_backend:
            mm_client = mm.mastermind_service_client(mm_hosts)
            remove_future_backend_records_on_fs(mm_client, os.path.normpath(options.remove_future_backend))
        elif options.check_path:
            mm_client = mm.mastermind_service_client(mm_hosts)
            out, code = check_path(mm_client, os.path.normpath(options.check_path) + '/')
            print out
            sys.exit(code)
        elif options.detach_path and options.detach_group:
            detach_node(mm_hosts, options.detach_path, int(options.detach_group))
        else:
            raise
    except Exception:
        l.exception('Failed')
        sys.exit(8)
