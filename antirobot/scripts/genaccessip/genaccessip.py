from __future__ import print_function
import sys
import re
import certifi
import urllib3
import time
import dns.resolver
import json
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

RACK_HOST = 'racktables.yandex.net'
NETWORK_TIMEOUT_SECONDS = 10
NUM_ATTEMPTS = 20
REQ_DELAY = 3
HTTP = urllib3.PoolManager(cert_reqs='CERT_REQUIRED', ca_certs=certifi.where())


def expand_macro(macro):
    print("Expanding %s..." % macro, file=sys.stderr)

    url = 'https://%(host)s/export/expand-fw-macro.php?macro=%(macro)s' % {'host': RACK_HOST, 'macro': macro}

    data = None
    for _ in range(NUM_ATTEMPTS):
        r = HTTP.request('GET', url)

        if r.status == 200:
            data = r.data.decode("utf-8")
            break

        if r.status in (429, 502):
            time.sleep(REQ_DELAY)
            continue

        raise Exception("Bad status code from racktables: {}. "
                        "Error message: {}. Trying to expand macro '{}'".format(r.status, r.data, macro))

    print("done", file=sys.stderr)

    return data


regIp = r'\d{1,4}\.\d{1,4}\.\d{1,4}\.\d{1,4}'
_reIp4 = re.compile(r'^%(ip)s(?:/\d+|\s*-\s*%(ip)s)?$' % {'ip': regIp})  # ip, ip-net or ip-range


def is_ip4(text):
    return _reIp4.match(text) is not None


_reHost = re.compile(r'^(?:[\w_\-]+\.)+[a-zA-Z]+$')


def is_host(text):
    return _reHost.match(text) is not None


_reMacro = re.compile(r'^_[a-zA-Z]\w+_$')


def is_macro(text):
    return _reMacro.match(text) is not None


_reUrl = re.compile(r'^(?:ftp|http|https)://\S+$')


def is_url(txt):
    return _reUrl.match(txt)


def is_valid_ip6_addr(ip_str):
    try:
        addr_parts = ip_str.strip().split('/', 1)
        if len(addr_parts) > 1:
            net = int(addr_parts[1])
            if net < 0 or net >= 128:
                return False

        sub_parts = addr_parts[0].split('::')
        if len(sub_parts) > 2:
            return False

        frags = []
        for p in sub_parts:
            if p:
                frags.extend(p.split(':'))

        if len(frags) > 8:
            return False

        for f in frags:
            try:
                val = int(f, 16)
                if val > 0xffff:
                    return False

            except ValueError:
                return False

        return True
    except ValueError:
        return False


def get_all_ips(hostname):
    my_resolver = dns.resolver.Resolver()
    my_resolver.nameservers = ['2a02:6b8:0:3400::1', '2a02:6b8::1:1']
    ips = []
    ips.extend(map(str, my_resolver.query(hostname, 'A', raise_on_no_answer=False)))     # IPv4
    ips.extend(map(str, my_resolver.query(hostname, 'AAAA', raise_on_no_answer=False)))  # IPv6
    return ips


def get_host_ip(hostname):
    print("Getting IP for host %s..." % hostname, file=sys.stderr)

    try:
        ips = get_all_ips(hostname)

        if len(ips) == 0:
            print("%s does not have any IP address" % hostname, file=sys.stderr)
            return None

        print("done", file=sys.stderr)
        return '\n'.join(ips)
    except dns.resolver.NXDOMAIN:
        print("%s does not exist" % hostname, file=sys.stderr)
        return None


def is_string(string):
    try:
        str(string)
        return True
    except ValueError:
        return False


def parse_json_data(d):
    """
        Get all ips from json
    """
    ip_list = ""
    for value in d:
        if isinstance(d, dict):
            value = d[value]
        new_ips = ""
        if isinstance(value, dict):
            new_ips = '\n' + parse_json_data(value)
        elif isinstance(value, list):
            new_ips = '\n' + parse_json_data(value)
        elif is_string(value) and is_ip4(str(value)):
            new_ips = '\n' + value
        elif is_string(value) and is_valid_ip6_addr(str(value)):
            new_ips = '\n' + value
        ip_list += new_ips
    return ip_list


def get_from_json(json_data):
    ips = parse_json_data(json_data)
    res = '\n'.join([x for x in ips.split('\n') if len(x) > 0])
    return res


def get_from_url(url):
    print("Fetching from %s..." % url, file=sys.stderr)
    http_request = HTTP.request('GET', url, timeout=NETWORK_TIMEOUT_SECONDS)
    data = http_request.data.decode("utf-8")
    headers = http_request.headers
    if "content-type" in headers and "application/json" in headers["content-type"]:
        json_data = json.loads(data)
        res = get_from_json(json_data)
    else:
        lines = data.strip().split('\n')
        res = '\n'.join([x.strip().split(None, 1)[0] for x in lines])

    print("done", file=sys.stderr)

    return res


def get_line_content(line):
    fs = line.strip().split('#', 1)
    if not fs:
        return '', ''

    if len(fs) == 1:
        return fs[0].strip(), ''

    return fs[0].strip(), fs[1].strip()


def translate_once(text):
    lines = [x for x in text.split('\n') if x]

    need_translate = False
    res = StringIO()

    for line in lines:
        i, comment = get_line_content(line)

        if comment:
            print('#', comment, file=res)

        if not i:
            continue

        if is_ip4(i):
            print(i, file=res)
        elif is_host(i):
            ip_str = get_host_ip(i)
            if ip_str:
                print('# %s' % i, file=res)
                print(ip_str, file=res)
            else:
                print('# %s [failed]' % i, file=res)

        elif is_macro(i):
            print('### BEGIN %s' % i, file=res)
            print(expand_macro(i), file=res)
            print('### END %s' % i, file=res)
            need_translate = True

        elif is_url(i):
            print('### BEGIN %s' % i, file=res)
            print(get_from_url(i), file=res)
            print('### END %s' % i, file=res)
            need_translate = True

        elif is_valid_ip6_addr(i):
            print(i, file=res)

        else:
            raise Exception("Unknown item type: %s" % i)

    return res.getvalue(), need_translate


def translate(text):
    while True:
        text, need_translate = translate_once(text)
        if not need_translate:
            return text
