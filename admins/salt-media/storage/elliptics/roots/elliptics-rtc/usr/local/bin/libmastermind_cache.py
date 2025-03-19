#!/usr/bin/python

import sys
import re

host_re = re.compile("((?<=host: )|(?<=locator )).+yandex.net(?=:\d{4,})")
error_re = re.compile("(?<=libmastermind: ).+")
time_re = re.compile("(?<=time: )\d+(?= milliseconds)")
expire_re = re.compile("cache \"(.+)\" (has been expired|will be expired soon|is too old|will expire soon|has expired); life-time=(\d+)s")


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("--out", help="[ monrun - Monrun. print - out cache. Default - %default ]", default='monrun', type=str)
    parser.add_option("--ignore", help="[ Default - %default ]", default='', type=str)
    parser.add_option("--warn_time", help="[ Default - %default ]", default=1800, type=int)
    parser.add_option("--crit_time", help="[ Default - %default ]", default=3600, type=int)
    (options, arguments) = parser.parse_args(argv)
    return options


def check_cache(options):
    ignore = options.ignore.split(',')
    start = False
    errors = []
    res = []
    cache = []
    host = ''
    loop = 0
    for line in sys.stdin:
        if 'collect_info_loop: begin; current host' in line:
            start = True
            loop += 1
            host = host_re.search(line).group(0)
        elif 'reconnect: connected to mastermind via locator' in line:
            start = True
            loop += 1
            host = host_re.search(line).group(0)
        elif 'collect_cached_keys' in line or \
            'serialize: skip saving cache: data not changed' in line or \
            'get_cached_keys' in line:
            continue
        elif 'error' in line or 'skip saving cache' in line and start:
            e = error_re.search(line).group(0)
            errors.append(e)
        elif 'trying to connect to locator ' in line and start:
            start = False
            res.append({'host': host, 'errors': errors, 'loop': loop, 'good': False})
            errors = []
            cache = []
        elif 'collect_info_loop: end; current host' in line and start:
            time = time_re.search(line).group(0)
            start = False
            res.append({'host': host, 'errors': errors, 'time': time, 'cache': cache, 'loop': loop, 'good': True})
            errors = []
            cache = []
        expire = expire_re.search(line)
        if expire:
            if expire.group(1) not in ignore:
                cache.append({'key': expire.group(1), 'message': expire.group(2), 'time': expire.group(3)})

    return res


def monrun(cache, options):
    bad_cache = []
    good_cache = []
    for c in cache:
        if c['good'] is True:
            good_cache.append(c)
        else:
            bad_cache.append(c)

    if good_cache:
        c = sorted(good_cache, key=lambda elem: elem['loop'])[-1]
        if 'cache' in c and c['cache']:
            msg = ''
            code = 0
            for info in c['cache']:
                if int(info['time']) > options.crit_time:
                    code = setlevel(2, code)
                    msg += '{0} {1} {2} > {3};'.format(info['key'], info['message'], info['time'], options.crit_time)
                elif int(info['time']) > options.warn_time:
                    code = setlevel(1, code)
                    msg += '{0} {1} {2} > {3};'.format(info['key'], info['message'], info['time'], options.warn_time)
            if not msg:
                msg = 'Ok'
            die(code, msg)
        if 'errors' in c and c['errors']:
            errs = [str(x) for x in c['errors']]
            err_str = ','.join(errs)
            if 'skip saving cache' in err_str:
                die(2, err_str)
            else:
                die(1, err_str)
        else:
            die(0, 'Ok')
    else:
        if not bad_cache:
            die(0, "Log file empty")
        else:
            c = sorted(bad_cache, key=lambda elem: elem['loop'])[0]
            errs = c['errors']
            die(2, ','.join(errs))


def setlevel(code, level):
    if code > level:
        return code
    else:
        return level


def die(code, msg):
    print "{0}; {1}".format(code, msg)
    sys.exit()

if __name__ == '__main__':
    options = parseArgs(sys.argv)
    cache = check_cache(options)
    if options.out == 'monrun':
        monrun(cache, options)
    elif options.out == 'print':
        print cache

