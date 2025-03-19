#!/usr/bin/python

import sys
import re

host_re = re.compile("((?<=host: )|(?<=locator )).+yandex.net||\[::\](?=:\d{4,})")
error_re = re.compile("(?<=libmastermind).+")
no_weights_re = re.compile("(?<=libmastermind: ).+namespace=([\w-]+): no couple weights")
ns_re = re.compile("(?<=libmastermind: ).+namespace=([\w-]+):.+")
time_re = re.compile("(?<=time: )\d+(?= milliseconds)")
expire_re = re.compile("cache \"(.+)\" (has been expired|will be expired soon|is too old|will expire soon|has expired); life-time=(\d+)s")


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("--out", help="[ monrun - Monrun. print - out cache. Default - %default ]", default='monrun', type=str)
    parser.add_option("--ignore", help="[ Default - %default ]", default='', type=str)
    parser.add_option("--ignore_namespaces", help="[ Default - %default ]", default='', type=str)
    parser.add_option("--warn_time", help="[ Default - %default ]", default=1800, type=int)
    parser.add_option("--crit_time", help="[ Default - %default ]", default=3600, type=int)
    (options, arguments) = parser.parse_args(argv)
    return options


def check_cache(options):
    ignore = options.ignore.split(',')
    ignore_namespaces = options.ignore_namespaces.split(',')
    start = False
    errors = []
    no_weights_ns = []
    res = []
    cache = []
    host = ''
    loop = 0
    for line in sys.stdin:
        if 'collect_info_loop: begin; current host' in line:
            start = True
            loop += 1
            if 'current host: none' in line:
                host = 'None'
            else:
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
            ns = ns_re.search(line)
            if ns:
                ns = ns.groups()[0]
                skip = False
                for i_n in ignore_namespaces:
                    if ns == i_n:
                        skip = True
                        break
                if skip:
                    continue
            no_weights = no_weights_re.search(line)
            if no_weights:
                no_weights_ns.append(no_weights.groups()[0])
            else:
                if 'cannot obtain namespace_state for couple' not in line:
                    errors.append(e)
        elif 'trying to connect to locator ' in line and start:
            start = False
            res.append({'host': host, 'errors': errors, 'loop': loop, 'good': False})
            errors = []
            cache = []
        elif 'collect_info_loop: end; current host' in line and start:
            time = time_re.search(line).group(0)
            start = False
            res.append({'host': host, 'errors': errors, 'time': time, 'cache': cache, 'loop': loop, 'good': True, 'no_weights': no_weights_ns})
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
        msg = ''
        code = 0
        if 'cache' in c and c['cache']:
            for info in c['cache']:
                if int(info['time']) > options.crit_time:
                    code = setlevel(2, code)
                    msg += '{0} {1} {2} > {3};'.format(info['key'], info['message'], info['time'], options.crit_time)
                elif int(info['time']) > options.warn_time:
                    code = setlevel(1, code)
                    msg += '{0} {1} {2} > {3};'.format(info['key'], info['message'], info['time'], options.warn_time)

        if 'errors' in c and c['errors']:
            ignore_errors = ['namespace: couples list is not set or empty', ', handler: get_storage_state_snapshot, trace_id: ']

            errs = []
            for x in c['errors']:
                ignore = False
                for ignore_error in ignore_errors:
                    if ignore_error in x:
                        ignore = True
                if not ignore:
                    errs.append(x)

            err_str = ','.join(errs)

            if err_str:
                code = setlevel(2, code)
                msg += err_str

        if 'no_weights' in c and c['no_weights']:
            msg += "No couple weights: "
            msg += ', '.join(c['no_weights'])
            code = setlevel(2, code)

        if not msg:
            msg = 'Ok'

        die(code, msg)
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
