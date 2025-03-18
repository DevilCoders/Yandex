import os
import sys
import time
import subprocess
import shutil
import string
import pwd
from collections import defaultdict
import fcntl
import urllib2
import logging
import simplejson
import ssl
import xmlrpclib
import tempfile
import socket
import requests
import re
import datetime

import config
from core.exceptions import UtilRuntimeException
from gaux.aux_colortext import red_text, blue_text, green_text
import gaux.aux_decorators
from collections import OrderedDict


GB = 1024 * 1024 * 1024
MB = 1024 * 1024


def prettify_size(v):
    if v > 100 * MB:
        return "{:.2f} Gb".format(float(v) / GB)
    else:
        return "{:.2f} Mb".format(float(v) / MB)


def prompt(question):
    while True:
        try:
            i = raw_input(red_text("%s (type 'yes' or 'no'): " % question))
        except KeyboardInterrupt:
            return False
        if i.lower() in ('yes', 'y'):
            return True
        elif i.lower() in ('no', 'n'):
            return False


def get_main_repo(verbose=False):
    from core.svnapi import SvnRepository

    path = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    return SvnRepository(path=path, verbose=verbose)


def rm_tree(path):
    def error_handler(func, path, excinfo):
        del func
        print "Failed to remove {}: {}.".format(path, excinfo[1])

    shutil.rmtree(path, onerror=error_handler)


def run_command(args, raise_failed=True, timeout=None, sleep_timeout=0.1, cwd=None, close_fds=False, stdin=None):
    """
        Wrapper on subprocess.Popen

        :return (int, str, str): return triplet of (returncoce, stdout, stderr)
    """

    if timeout is not None:
        sleep_timeout = min(timeout / 10, sleep_timeout)

    try:
        if stdin is None:
            p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd, close_fds=close_fds)
        else:
            p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd,
                                 close_fds=close_fds)
    except Exception, e:
        if raise_failed:
            raise Exception("subpocess.Popen for <<%s>> failed: %s" % (args, e))
        else:
            return 1, 'Got unknown error %s' % e, ''

    try:
        if stdin is not None:
            p.stdin.write(stdin)  # Is it correct ?
            p.stdin.close()

        if timeout is None:
            out, err = p.communicate()
            if p.returncode != 0 and raise_failed:
                raise Exception("Command <<%s>> returned %d\nStdout:%s\nStderr:%s" % (args, p.returncode, out, err))
        else:
            out, err = '', ''
            wait_till = time.time() + timeout

            fcntl.fcntl(p.stdout, fcntl.F_SETFL, fcntl.fcntl(p.stdout, fcntl.F_GETFL) | os.O_NONBLOCK)
            fcntl.fcntl(p.stderr, fcntl.F_SETFL, fcntl.fcntl(p.stderr, fcntl.F_GETFL) | os.O_NONBLOCK)

            while time.time() < wait_till:
                p.poll()
                try:
                    while True:
                        r = os.read(p.stdout.fileno(), 1024)
                        out += r
                        if len(r) == 0:
                            break
                except OSError:
                    pass
                try:
                    while True:
                        r = os.read(p.stderr.fileno(), 1024)
                        err += r
                        if len(r) == 0:
                            break
                except OSError:
                    pass
                if p.returncode == 0:
                    return p.returncode, out, err
                if p.returncode is not None:
                    if raise_failed:
                        raise Exception("Command <<%s>> returned %d\nStdout:%s\nStderr:%s" % (args, p.returncode, out, err))
                    else:
                        return p.returncode, out, err
                time.sleep(sleep_timeout)

            if raise_failed:
                raise Exception("Command <<%s>> timed out (%f seconds)" % (args, timeout))
    finally:
        if p.returncode is None:
            p.kill()
            p.wait()

    return p.returncode, out, err


def draw_hist(values, center=None, w=30, h=30, invert=False, header=''):
    values = [float(x) for x in values]
    min_value = min(values)
    max_value = max(values)
    if center is not None:
        min_value = min(min_value, center - (max_value - center))
        max_value = max(max_value, center - (center - min_value))
    if min_value == max_value:
        return
    assert (min_value != max_value)
    print 'min_value: %s' % min_value
    print 'max_value: %s' % max_value
    values = [(x - min_value) / (max_value - min_value) for x in values]
    values = [min(1.0, max(x, .0)) for x in values]

    # assert(all(0.0 <= x <= 1.0 for x in values))
    hist = [0 for x in range(w)]
    max_count = 0
    for v in values:
        i = (w - 1) * v
        hist[int(i)] += 1
        max_count = max(max_count, hist[int(i)])
    if invert:
        hist = list(reversed(hist))
    for i in range(w):
        hist[i] = hist[i] * h / max_count
    print '=' * (w if not header else len(header))
    for lvl in range(h - 1, -1, -1):
        line = [('*' if hist[x] >= lvl else ' ') for x in range(w)]
        line = ''.join(line)
        print line
    if header:
        print header
    print '=' * (w if not header else len(header))


def freq(iterator, map_f=None):
    counter = defaultdict(int)
    if map_f is None:
        for x in iterator:
            counter[x] += 1
    else:
        for x in iterator:
            counter[map_f(x)] += 1
    return dict(counter)


def indent(text, ind=' ' * 4):
    lines = text.split('\n')
    return '\n'.join([ind + line for line in lines])


def raise_extended_exception(src_exception, info_msg):
    msg = '{}\n    Got exception of type <{}>: {}'.format(info_msg, type(src_exception), str(src_exception))
    raise Exception(msg)


def getlogin():
    # By some reason os.getlogin() can return empty string
    # and is not recommended
    return pwd.getpwuid(os.getuid())[0]


def unicode_to_str(src):
    if src.__class__ == str:
        return src
    return src.encode('utf8', 'replace')


def str_to_unicode(src):
    if src.__class__ == unicode:
        return src
    return src.decode('utf8', 'replace')


def transform_plain_obj(src, f):
    # time critical function
    if src.__class__ in (unicode, str):
        return f(src)
    elif src.__class__ in (int, float, bool):
        return src
    elif src.__class__ in (dict,):
        # we can have both dict and ordered_dict here
        return dict((x, transform_plain_obj(y, f)) for x, y in src.iteritems())
    elif src.__class__ in (list,):
        return [transform_plain_obj(x, f) for x in src]
    elif src.__class__ in (set,):
        return set(transform_plain_obj(x, f) for x in src)
    elif src is None:
        return src

    raise Exception('Unexpected type in json: %s (%s)' % (type(src), src))


def plain_obj_unicode_to_str(src):
    # time critical function, so make this func as fast as possible
    if src.__class__ == unicode:
        return src.encode('utf8', 'replace')
    elif src.__class__ in (str, int, float, bool):
        return src
    elif src.__class__ in (dict,):
        # we can have both dict and ordered_dict here
        return dict((x, plain_obj_unicode_to_str(y)) for x, y in src.iteritems())
    elif isinstance(src, dict):
        return (type(src))((x, plain_obj_unicode_to_str(y)) for x, y in src.iteritems())
    elif src.__class__ in (list,):
        return [plain_obj_unicode_to_str(x) for x in src]
    elif src.__class__ in (set,):
        return set(plain_obj_unicode_to_str(x) for x in src)
    elif src is None:
        return src

    raise Exception('Unexpected type in json: %s (%s)' % (type(src), src))


def plain_obj_str_to_unicode(src):
    # time critical function, so make this func as fast as possible
    if src.__class__ == str:
        return src.decode('utf8', 'replace')
    elif src.__class__ in (unicode, int, float, bool):
        return src
    elif src.__class__ == dict:
        # we can have both dict and ordered_dict here
        return dict((x, plain_obj_str_to_unicode(y)) for x, y in src.iteritems())
    elif isinstance(src, dict):
        return (type(src))((x, plain_obj_str_to_unicode(y)) for x, y in src.iteritems())
    elif src.__class__ in (list,):
        return [plain_obj_str_to_unicode(x) for x in src]
    elif src.__class__ in (set,):
        return set(plain_obj_str_to_unicode(x) for x in src)
    elif src.__class__ == long:
        return src
    elif src is None:
        return src

    raise Exception('Unexpected type in json: %s (%s)' % (type(src), src))


def ids_alphabet():
    return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_."


def check_group_name(name):
    if not name:
        raise Exception('Empty group name')

    alpha = set(string.uppercase) | {'_'} | set(string.digits)
    invalid_chars = set(name) - set(alpha)
    if invalid_chars:
        raise Exception("Invalid group name '%s', group name may contain only uppercase alphanumeric characters and "
                        "underscores. Bad chars: '%s'" % (name, ' '.join(invalid_chars)))


def mkdirp(path):
    path = os.path.abspath(path)
    path = path.split('/')
    cpath = '/'
    for segment in path:
        cpath = cpath + '/' + segment
        if not os.path.exists(cpath):
            os.mkdir(cpath)


def flat(data):
    if not isinstance(data, list):
        return [data]
    result = []
    for x in data:
        if not isinstance(x, list):
            result.append(x)
        else:
            result.extend(flat(x))
    return result


def int_to_smart_str(value):
    """ Human readable string """
    if value < 0:
        return "-%s" % _int_to_smart_str(-value)
    else:
        return _int_to_smart_str(value)


def _int_to_smart_str(value):
    isinstance(value, int)
    m = 1. * value
    e = 0
    while m / 1000. >= 1.:
        m /= 1000.
        e += 1

    if m >= 100.0:
        m_str = "%.0f" % m
    elif m >= 10.0:
        m_str = "%.1f" % m
    else:
        m_str = "%.2f" % m
    if m_str.find(".") != -1:
        m_str = m_str.rstrip("0")
        m_str = m_str.rstrip(".")

    if e == 0:
        return int(value)
    elif e <= 3:
        e_strs = ["", "K", "M", "B"]
        return "%s%s" % (m_str, e_strs[e])
    else:
        return "%se10+%s" % (m_str, e * 3)


def dict_to_options(src):
    class DummyOptions:
        def __init__(self):
            pass

    options = DummyOptions()
    for key in src:
        setattr(options, key, src[key])
    return options


def print_progress(curval, endval, barlen=100, stopped=False):
    if endval > 0:
        percent = int(round(float(curval) / endval * 100))
        hashes = '#' * (curval * barlen / endval)
    else:
        percent = 100
        hashes = '#' * barlen

    spaces = ' ' * (barlen - len(hashes))
    sys.stdout.write(blue_text("\rDone: [{0}] {1}%".format(hashes + spaces, percent)))
    if stopped:
        if curval == endval:
            sys.stdout.write(green_text(" FINISHED"))
            sys.stdout.write("\n")
        else:
            sys.stdout.write(red_text(" STOPPED"))
            sys.stdout.write("\n")
    sys.stdout.flush()


def get_last_release(repo, tag_prefix):
    last_release = 0

    for tag in repo.tags():
        if not tag.startswith(tag_prefix):
            continue

        rev_str = tag[len(tag_prefix):]
        pos = 0
        while pos < len(rev_str) and ('0' <= rev_str[pos] <= '9'):
            pos += 1
        rev_str = rev_str[:pos]

        try:
            release = int(rev_str)
            if release < 0:
                raise ValueError
        except ValueError:
            raise Exception("There is an invalid tag in the repository: {}.", tag)

        last_release = max(last_release, release)

    return last_release


def get_used_disk(data, db):
    seen_shards = set()
    seen_groups = set()
    total_size = 0
    for tier, shard_id, group_name in data:
        if (tier, shard_id) in seen_shards:
            continue
        else:
            seen_shards.add((tier, shard_id))

        group = db.groups.get_group(group_name)
        extra = group.properties.extra_disk_size
        extra_per_instance = group.properties.extra_disk_size_per_instance
        extra_disk_shards = group.properties.extra_disk_shards

        if group.name not in seen_groups:
            total_size += extra
            seen_groups.add(group.name)
        total_size += extra_per_instance
        total_size += extra_disk_shards * db.tiers.tiers[tier].disk_size
    return total_size


def get_worst_tier_by_size(s, mydb):
    tiers_data = map(lambda x: (x.split(':')[0], int(x.split(':')[1])), s.split(','))
    return max(map(lambda (tier, ss): mydb.tiers.tiers[tier].disk_size / ss, tiers_data))


class OAuthTransport(xmlrpclib.SafeTransport):
    def __init__(self, token):
        xmlrpclib.SafeTransport.__init__(self)
        self.token = token

    def send_request(self, connection, handler, request_body):
        xmlrpclib.SafeTransport.send_request(self, connection, handler, request_body)
        if self.token:
            connection.putheader("Authorization", 'OAuth %s' % self.token)


@gaux.aux_decorators.memoize
def memoize_urlopen(retries, *args, **kwargs):
    for i in range(retries):
        try:
            return urllib2.urlopen(*args, **kwargs).read()
        except Exception:
            pass


def retry_urlopen(retries, *args, **kwargs):
    SLEEP_TIMINGS = [1, 5, 10, 50, 200]
    for i in range(retries):
        try:
            return urllib2.urlopen(*args, **kwargs).read()
        except (urllib2.HTTPError, ssl.SSLError, urllib2.URLError) as e:
            if i < retries - 1:
                time.sleep(SLEEP_TIMINGS[min(i, len(SLEEP_TIMINGS) - 1)])
            else:
                url = args[0]
                if isinstance(url, urllib2.Request):
                    url = url.get_full_url()
                raise Exception("Got exception <%s> while processing url <%s> (%s) (%s)" % (e.__class__, url, str(e), str(e.headers)))
        except Exception:
            if i < retries - 1:
                time.sleep(SLEEP_TIMINGS[min(i, len(SLEEP_TIMINGS) - 1)])
            else:
                raise


def retry_requests_get(retries, *args, **kwargs):
    SLEEP_TIMINGS = [1, 5, 10, 50, 200]

    # suppress noisy warningis
    requests.packages.urllib3.disable_warnings()

    for i in range(retries):
        try:
            r = requests.get(*args, **kwargs)
            r.raise_for_status()
            return r
        except Exception as e:
            print '[{:.2f}]: Got exception <{}> while processing url <{}>, attempt {} ({})'.format(time.time(), e.__class__, args[0], i, str(e))

            if i < retries - 1:
                time.sleep(SLEEP_TIMINGS[min(i, len(SLEEP_TIMINGS) - 1)])
            else:
                url = args[0]
                raise Exception("Got exception <%s> while processing url <%s> (%s)" % (e.__class__, url, str(e)))


def read_tail_lines(fname, N):
    retcode, data, _ = run_command(["tail", "-%s" % N, fname])

    datalines = data.rstrip().split('\n')
    datalines = datalines[-N:]

    if len(datalines) == 1 and datalines[0] == '':
        return []
    return datalines


def correct_pfname(fname):
    fname = os.path.basename(fname)

    for postfix in ['.py', '.pyc']:
        if fname.endswith(postfix):
            fname = fname[:-len(postfix)]

    return fname


def get_corrected_power_usage(model, iusage, husage, correct=True):
    """
        Get power units of something with usage <iusage> (in portions of 1.0) and host usage (in portions of 1.0)

        Params:
            - model: cpu model (cpu model from core/hosts.py)
            - iusage: instance usage (in range [0.0, 1.0])
            - husage: total host usage (in range [0.0, 1.0])
            - correct: If True, make correction, otherwise return uncorrected values

        Returns:
            - power units, consumed by instance with iusage usage
    """

    if correct:
        ind = int(husage * model.ncpu)
        rcoeff = husage * model.ncpu - ind
        lcoeff = 1.0 - rcoeff

        if ind == model.ncpu:
            return model.power * iusage / husage
        else:
            powerbyload = model.powerbyload.tbon
            return (powerbyload[ind] * lcoeff + powerbyload[ind + 1] * rcoeff) * iusage / husage
    else:
        return iusage * model.power


def setup_logger_logfile(logger, fname, lvl, log_format="%(message)s"):
    """
        Link file <fname> to logging.Logger <logger> with specified <lvl>

        :type logger: logging.Logger
        :type fname: str
        :type lvl: int
        :type log_format: str

        :param logger: destination logger
        :param fname: output filename for logger
        :param lvl: logging level, e. g. Logging.DEBUG, Logging.INFO
        :param log_format: custom formatter for log

        :return: return none, modify logger inplace
    """

    mkdirp(os.path.dirname(fname))
    if not os.path.exists(fname):
        open(fname, 'w').close()
        os.chmod(fname, 0777)

    handler = logging.FileHandler(fname)
    handler.setLevel(lvl)

    # change comma to dot in time format
    class MyFormatter(logging.Formatter):
        def formatTime(self, record, datefmt=None):
            ct = self.converter(record.created)
            if datefmt:
                s = time.strftime(datefmt, ct)
            else:
                t = time.strftime("%Y-%m-%d %H:%M:%S", ct)
                s = "%s.%03d" % (t, record.msecs)
            return s

    formatter = MyFormatter(log_format)
    handler.setFormatter(formatter)

    logger.addHandler(handler)


def unicode_to_utf8(data):
    """
        Convert all object unicode strings in complex object to usual utf8 strings.

        :param data: complex object to convert
        :return: object without unicode strings
    """
    if isinstance(data, dict):
        return {unicode_to_utf8(key): unicode_to_utf8(value) for key, value in data.iteritems()}
    elif isinstance(data, list):
        return [unicode_to_utf8(element) for element in data]
    elif isinstance(data, unicode):
        return data.encode('utf-8')
    else:
        return data


def load_nanny_url(path, body=None):
    from core.settings import SETTINGS

    url = "%s%s" % (SETTINGS.services.nanny.rest.url, path)
    nanny_request = urllib2.Request(url)
    nanny_request.add_header("Authorization", "OAuth %s" % config.get_default_oauth())
    nanny_request.add_header("Accept", "application/json")
    if body is not None:
        nanny_request.add_header("Content-Type", "application/json")

    response = retry_urlopen(5, nanny_request, timeout=10)
    if response is None:
        raise UtilRuntimeException(correct_pfname(__file__), "No correct reply from <%s>" % url)

    return simplejson.loads(response)#.read())


def transform_tree(obj, func, raise_notfound=True):
    """Transform complex struct like multilevel dict based on result of transform_func

    Transformation is performed by recursive traverse of obj and checking if node should be transformed.
    No extra checks (like dict keys are transformed to different objects) are performed. Processing of
    custom object types is not supported.

    :param obj: arbitrary python object to process
    :type obj: python object
    :param func: func to transform any object to something new
    :type func: python function
    :param raise_notfound: should we raise exception if found class we do not know how to process
    :type raise_notfound: bool

    :return: new object, created by transformation of obj
    """
    # check if this node is transformed
    transformed, value = func(obj)
    if transformed:
        return value

    # go recursive
    if obj.__class__ in (str, int, float, bool):
        return obj
    elif obj.__class__ in (dict, OrderedDict):
        result = dict()
        for k, v in obj.iteritems():
            result[transform_tree(k, func, raise_notfound=raise_notfound)] = transform_tree(v, func, raise_notfound=raise_notfound)
        return result
    elif obj.__class__ in (list, tuple):
        return obj.__class__((transform_tree(x, func, raise_notfound=raise_notfound) for x in obj))
    elif obj is None:
        return obj
    else:
        if raise_notfound:
            raise Exception('Unknown type <{}> in transfrom_tree function'.format(obj.__class__.__name__))
        else:
            return obj


def download_sandbox_resource(resource_id, path=None, timeouts=[1, 5, 10, 50, 500]):
    """Download sandbox resource to specified path"""
    # download resource
    tmp_dir = tempfile.mkdtemp()
    os.chmod(tmp_dir, 0o777)

    # try with copier
    try:
        import api.copier
        copier = api.copier.Copier()
        copier.get('sbr:{}'.format(resource_id), tmp_dir, network=api.copier.Network.Backbone).wait()
        resource_name = os.listdir(tmp_dir)[0]
    except Exception:
        # try with proxy
        for timeout in (1, 5, 10, 100, 500):
            try:
                resource = request_sandbox('/resource/{}'.format(resource_id))
                break
            except socket.error:
                time.sleep(timeout)
        else:
            raise Exception('Failed to fetch resource <{}>'.format(resource_id))

        resource_http_id = resource['http']['proxy']
        resource_name = resource['file_name']

        with open(os.path.join(tmp_dir, resource_name), 'w') as f:
            f.write(retry_urlopen(5, resource_http_id))

    if path is None:
        return open(os.path.join(tmp_dir, resource_name)).read()
    else:
        shutil.move(os.path.join(tmp_dir, resource_name), path)


def get_last_sandbox_resource(resource_type):
    """Get last sandbox resource of specified type as text string"""
    from core.settings import SETTINGS

    url = '{}/resource?limit=1&type={}'.format(SETTINGS.services.sandbox.rest.url, resource_type)
    data = simplejson.loads(retry_urlopen(3, url))
    resource_id = data['items'][0]['id']

    return download_sandbox_resource(resource_id, path=None)


def wait_sandbox_task(task_id, timeout=300):
    from core.settings import SETTINGS

    wait_till = time.time() + timeout
    while time.time() < wait_till:
        try:
            status = request_sandbox('task/{}'.format(task_id), headers={})['status']
        except:
            time.sleep(1)
            continue

        if status not in ['ENQUEUED', 'ENQUEUING', 'EXECUTING', 'PREPARING', 'FINISHING', 'TEMPORARY', 'DRAFT', 'ASSIGNED']:
            if status == 'SUCCESS':
                break
            raise Exception("Task %s%d exited with status %s" % (SETTINGS.services.sandbox.http.tasks.url, task_id, status))

        time.sleep(1)
    if time.time() > wait_till:
        raise Exception("Task %s%d did not finished" % (SETTINGS.services.sandbox.http.tasks.url, task_id))


def get_instance_used_ports(db, instance):
    used_ports = set()
    group = db.groups.get_group(instance.type)

    # instance ports
    if group.card.legacy.funcs.instancePort.startswith('new'):
        for i in xrange(8):
            used_ports.add(instance.port + i)
    else:
        used_ports.add(instance.port)

    # monigoring/juggler ports
    if group.card.properties.monitoring_juggler_port is not None:
        used_ports.add(group.card.properties.monitoring_juggler_port)
    if group.card.properties.monitoring_golovan_port is not None:
        used_ports.add(group.card.properties.monitoring_golovan_port)

    return used_ports


def get_group_used_ports(group):
    used_ports = set()

    port_func = group.card.legacy.funcs.instancePort

    m = re.match('new(\d+)', port_func)
    if m:
        for i in xrange(8):
            used_ports.add(int(m.group(1)) + i)

    m = re.match('old(\d+)', port_func)
    if m:
        used_ports.add(int(m.group(1)))

    m = re.match('default', port_func)
    if m:
        used_ports.add(8041)

    # monigoring/juggler ports
    if group.card.properties.monitoring_juggler_port is not None:
        used_ports.add(group.card.properties.monitoring_juggler_port)
    if group.card.properties.monitoring_golovan_port is not None:
        used_ports.add(group.card.properties.monitoring_golovan_port)

    return used_ports


def request_sandbox(path, data=None, method='GET', headers=None):
    from core.settings import SETTINGS

    requests.packages.urllib3.disable_warnings()

    url = '{}/{}'.format(SETTINGS.services.sandbox.rest.url, path)

    if headers is None:
        headers = {}
        if method in ('POST', 'PUT', 'DELETE'):  # add auth for change operations
            headers['Authorization'] = 'OAuth {}'.format(config.get_default_oauth())
        if data is not None:  # support json
            headers['Content-Type'] = 'application/json'

    response = requests.request(method=method, url=url, data=data, headers=headers)

    if response.status_code in (200, 201):
        return response.json()
    elif response.status_code == 204:
        return None
    else:
        raise Exception('Got status code <{}> with text <{}> when requesting <{}> with headers <{}> and method <{}>'.format(
            response.status_code, response.text[:100], url, headers, method
        ))


def to_datetime(timestamp):
    formats = ('%Y-%m-%dT%H:%M:%S', '%Y-%m-%dT%H:%M:%S.%fZ', '%Y.%m.%dT%H:%M:%S', '%Y.%m.%dT%H:%M:%S.%fZ')
    for i, timstamp_format in enumerate(formats):
        try:
            return datetime.datetime.strptime(timestamp, timstamp_format)
        except Exception as e:
            if i >= len(formats) - 1:
                raise
