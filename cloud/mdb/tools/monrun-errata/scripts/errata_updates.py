#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Provides: errata_updates

import argparse
import datetime
import gzip
import os
import platform
import socket
import sys
import time
import urllib2

if sys.hexversion < 0x02060000:
    import simplejson as json
else:
    import json

parser = argparse.ArgumentParser()

parser.add_argument('-u', '--url',
                    type=str,
                    default='https://errata.s3.yandex.net/errata.json.gz',
                    help='Errata file url [https://errata.s3.yandex.net/errata.json.gz]')

parser.add_argument('-m', '--max-attempts',
                    type=int,
                    default=3,
                    help='max number of retries to get data from URL above [3 times]')

parser.add_argument('-o', '--timeout',
                    type=int,
                    default=10,
                    help='timeout for each of retries [10 seconds]')

parser.add_argument('-t', '--tmp',
                    type=str,
                    default='/tmp',
                    help='tmp dir for proccessing errata file [/tmp]')

parser.add_argument('-v', '--verbose',
                    action="store_true",
                    help='list pending updates one line for each')

parser.add_argument('-s', '--send',
                    type=str,
                    default='none',
                    help='Should the data be send to a syslog-server [none]. Format: fqdn:port/protocol. Example: syslog.company.ru:514/tcp')

parser.add_argument('-w', '--warn-days',
                    type=int,
                    default=0,
                    help='Emit warn for updates older than N days')

parser.add_argument('-c', '--crit-days',
                    type=int,
                    default=float('inf'),
                    help='Emit crit for updates older than N days')

args = parser.parse_args()


def die(code=0, comment="OK"):
    if code == 0:
        print 'PASSIVE-CHECK:errata_updates;0;OK'
    else:
        print 'PASSIVE-CHECK:errata_updates;%d;%s' % (code, comment)
    sys.exit(0)


def apt_version_compare(apt_pkg, left, right):
    """
    Apt version compare helper
    """
    if ':' in left and ':' not in right:
        return apt_pkg.version_compare(left.split(':')[1], right)
    if ':' not in left and ':' in right:
        return apt_pkg.version_compare(left, right.split(':')[1])

    return apt_pkg.version_compare(left, right)


def linux_image_ubuntu_fix(pkg):
    return '-'.join(pkg.split('-')[:3] + pkg.split('-')[4:])


def parse_syslog_url(url):
    host, port, protocol = url.replace(':', ' ').replace('/', ' ').split()
    # TODO: should we add some checks here?
    return (host, int(port), protocol)


def get_url(req):
    attempt = 1
    url = None
    while attempt <= args.max_attempts:
        try:
            url = urllib2.urlopen(req, timeout=args.timeout)
        except urllib2.URLError as e:
            pass
        if url and url.getcode() == 200:
            return url
        else:
            attempt += 1
        if attempt > args.max_attempts:
            raise e

try:
    warn_thresh = time.time() - args.warn_days * 86400
    crit_thresh = time.time() - args.crit_days * 86400

    os.chdir(args.tmp)

    req = urllib2.Request(args.url)
    req.get_method = lambda: 'HEAD'

    url = get_url(req)

    meta = url.info()

    url_time = time.mktime(meta.getdate('last-modified'))

    url_size = int(meta.getheader("Content-Length"))

    file_name = args.url.split('/')[-1]

    try:
        file_time = time.ctime(os.path.getmtime(file_name))

        f = open(file_name, 'rb')

        file_size = len(f.read())

        f.close()

        if file_time <= url_time or file_size != url_size:
            raise Exception('outdated')
    except Exception:
        f = open(file_name, 'wb')
        f.write(get_url(args.url).read())
        f.close()

    f = gzip.open(file_name)
    d = json.loads(f.read())
    f.close()

    dist = platform.dist()

    res = []
    pkgs = {}

    if dist[0] == 'Ubuntu':
        import apt_pkg
        apt_pkg.init()

        os_version = 'Ubuntu ' + dist[1]
        splitted = dist[1].split('.')
        if int(splitted[0]) % 2 == 0 and splitted[1] == '04':
            os_version += ' LTS'
        if os_version in d:
            errata_list = d[os_version]
        else:
            die(0, 'OK')

        f = open('/var/lib/dpkg/status')
        cur_pkg_name = None
        for line in f:
            if line.startswith('Package: '):
                cur_pkg_name = line.replace('Package: ', '').rstrip()
                # Unfortunately Ubuntu guys set kernel version in pkg name
                # so we just remove useless part of it here
                if 'linux-image' in cur_pkg_name:
                    if os.uname()[2].split('-')[0] in cur_pkg_name:
                        cur_pkg_name = linux_image_ubuntu_fix(cur_pkg_name)
                    else:
                        cur_pkg_name = None
            elif cur_pkg_name is not None and line.startswith('Status: '):
                if 'install ok installed' not in line:
                    # Skip deinstalled packages
                    cur_pkg_name = None
            elif cur_pkg_name is not None and line.startswith('Version: '):
                cur_version = line.replace('Version: ', '').rstrip()
                if cur_pkg_name in pkgs:
                    if apt_pkg.version_compare(cur_version,
                                               pkgs[cur_pkg_name]) > 0:
                        pkgs[cur_pkg_name] = cur_version
                else:
                    pkgs[cur_pkg_name] = cur_version
                cur_pkg_name = None
        f.close()

        for i in errata_list:
            if i['pkg'] in pkgs:
                if apt_version_compare(apt_pkg, i['version'], pkgs[i['pkg']]) > 0:
                    res.append((i['pkg'] + ', ' +
                                i['version'] + ': ' +
                                i['issue'], i['timestamp']))
            elif linux_image_ubuntu_fix(i['pkg']) in pkgs:
                if apt_pkg.version_compare(
                        i['version'],
                        pkgs[linux_image_ubuntu_fix(i['pkg'])]) > 0:
                    res.append((i['pkg'] + ', ' +
                                i['version'] + ': ' +
                                i['issue'], i['timestamp']))
    else:
        os_version = 'RHEL ' + dist[1].split('.')[0]
        if os_version in d:
            errata_list = d[os_version]
        else:
            die(0, 'OK')

        import yum
        import rpm
        yb = yum.YumBase()
        yb.conf.cache = 1
        for pkg in yb.rpmdb.returnPackages():
            if pkg.name in pkgs:
                if rpm.labelCompare(('1', pkg.version, pkg.release),
                                    pkgs[pkg.name]) > 0:
                    pkgs[pkg.name] = ('1', pkg.version, pkg.release)
            else:
                pkgs[pkg.name] = ('1', pkg.version, pkg.release)

        for i in errata_list:
            if i['pkg'] in pkgs:
                upd_tuple = ('1', i['version'], i['release'])
                for arch in ['x86_64', 'i686', 'noarch']:
                    if arch in upd_tuple[2] and arch not in pkgs[i['pkg']][2]:
                        upd_tuple = ('1', i['version'],
                                     i['release'].replace('.' + arch, ''))
                if rpm.labelCompare(upd_tuple, pkgs[i['pkg']]) > 0:
                    res.append((i['pkg'] + ', ' +
                                i['version'] + '-' +
                                i['release'] + ': ' +
                                i['issue'], i['timestamp']))

    if args.send != "none":
        host, port, protocol = parse_syslog_url(args.send)
        # Just in case: we always use udp, if there is no direct point
        socktype = socket.SOCK_STREAM if protocol == "tcp" else socket.SOCK_DGRAM
        sock = None
        for answer in socket.getaddrinfo(host, port, socket.AF_UNSPEC, socktype):
            af, stype, proto, canonname, sa = answer
            try:
                sock = socket.socket(af, stype, proto)
            except socket.error as msg:
                sock = None
                continue
            try:
                sock.connect(sa)
            except socket.error as msg:
                sock.close()
                sock = None
                continue
            break
        if sock is None:
            die(1, 'could not open syslog socket')

        # This dict is a template for syslog's header, witch filled according to RFC-5424
        # https://tools.ietf.org/html/rfc5424#section-6
        h = {
            'prival': {
                'facility': 10,  # security/authorization messages (authpriv)
                'severity': 2  # Critical: critical conditions
            },
            'version': '1',
            'timestamp': datetime.datetime.utcnow().isoformat("T"),
            'hostname': socket.gethostname(),
            'app-name': os.path.basename(__file__),
            'procid': str(os.getpid()),
            'msgid': '-'
        }
        pri = str(h['prival']['facility'] * 8 + h['prival']['severity'])
        #header = '<' + pri + '>' + h['version'] + ' ' + h['timestamp'] + ' ' + h['hostname'] + ' ' + h['app-name'].replace('.py',':') + ' ' + h['procid'] + ' ' + h['msgid']
        header = '<' + pri + '>' + h['app-name'].replace('.py',':') + ' '
        if res:
            for event in res:
                # Previously in the code
                # For RHEL: res.append(i['pkg'] + ', ' + i['version'] + '-' + i['release'] + ': ' + i['issue'])
                # For Ubuntu: res.append(i['pkg'] + ', ' + i['version'] + ': ' + i['issue'])
                # For the simplicity, we assume RHEL version includes release within itself
                data = event[0].split()
                mes = header + 'package=' + data[0][:-1] + ' version=' + data[1][:-1] + ' errata=' + data[2]
                sock.sendall(mes)
        else:
            mes = header + 'security_updates=ok'
            sock.sendall(mes)
        sock.close()

    level = 0
    filtered = set()
    for event in res:
        if warn_thresh >= event[1]:
            level = max(level, 1)
            filtered.add(event[0])
        if crit_thresh >= event[1]:
            level = max(level, 2)
            filtered.add(event[0])

    result = sorted(filtered)

    if not args.verbose:
        if result:
            if len(filtered) > 50:
                message = str(len(result)) + ' updates(s) available'
            else:
                message = ', '.join(result)
            die(level, message)
        else:
            die(0, 'OK')
    else:
        for i in result:
            print i
except Exception, e:
    die(1, 'Unable to get errata updates info: ' + str(e))
