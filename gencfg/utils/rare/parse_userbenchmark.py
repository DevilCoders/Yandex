#!/skynet/python/bin/python
"""
    Parse hdd/ssd page from userbenchmark.com (extract random/sequential speed and some other stuff)
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re
import pprint
from collections import OrderedDict

import gencfg
from core.argparse.parser import ArgumentParserExt


def get_parser():
    parser = ArgumentParserExt(description="Parse userbenchmark.com page")
    parser.add_argument("-i", "--input-file", type=str, required=True,
            help="Obligatory. Html file to parse")

    return parser


def detect_vendor(content):
    m = re.search('searchTerm=Brand:(.*?)"', content)
    if m:
        return m.group(1)
    return "Unknown"


def detect_url(content):
    m = re.search('stealthlink.*href="(.*?)"', content)
    if m:
        return m.group(1)
    return "Unknown"


def detect_model(content):
    m = re.search('stealthlink.*href=.*?>(.*?)<.*', content)
    if m:
        return m.group(1).rpartition(' ')[0]
    return "Unknown"


def detect_rota(content):
    m = re.search('stealthlink.*href="http://(.*?)\.', content)
    if m:
        if m.group(1) == 'ssd':
            return False
        elif m.group(1) == 'hdd':
            return True
        else:
            raise Exception, "Unknown rota type <%s>" % m.group(1)

    raise Exception, "Could not find rota type"

def detect_size(content):
    m = re.search('stealthlink.*href=.*?>(.*?)<.*', content)
    if m:
        sz = m.group(1).rpartition(' ')[2]
        if sz.endswith('TB'):
            return int(sz[:-2]) * 1024
        elif sz.endswith('GB'):
            return int(sz[:-2])
        else:
            return 0
    return 0


def detect_aliases(content):
    m = re.search('Models: (.*)', content)
    if m:
        models = map(lambda x: x.strip(), m.group(1).split(','))
    else:
        models = []

    return models


def detect_iops(content):
    result = OrderedDict()
    for mark, signal_name, converter in [
        ('Read ', 'sr', lambda x: int(float(x))),
        ('Write ', 'sw', lambda x: int(float(x))),
        ('4K Read ', 'rr', lambda x: int(float(x) * 1024 * 1024 / 4096)),
        ('4K Write ', 'rw', lambda x: int(float(x) * 1024 * 1024 / 4096)),
    ]:
        for rg in [
            'mcs-hl-col.*%s.*start-tag.*attribute-value"></a>"&gt;</span><span>(.*?)<.*' % mark,
            'mcs-hl-col.*?>.*%s<span class="">(.*?)<.*' % mark,
        ]:
            m = re.search(rg, content)
            if m:
                result[signal_name] = converter(m.group(1))
                break
        if not m:
            result[signal_name] = 0

    return result


def main(options):
    result = OrderedDict()

    content = open(options.input_file).read()

    result["model"] = detect_model(content)
    result["url"] = detect_url(content)
    result['vendor'] = detect_vendor(content)
    result['aliases'] = detect_aliases(content)
    result['size'] = detect_size(content)
    result['rota'] = detect_rota(content)
    result['iops'] = detect_iops(content)

    return result


def print_result(result, options):
    print "%s:" % result["model"]
    print "    size: %s" % result["size"]
    print "    rota: %s" % result["rota"]
    print "    url: %s" % result["url"]
    print "    vendor: %s" % result["vendor"]
    print "    aliases: %s" % ",".join(result["aliases"])
    print "    iops:"
    print "        rr: %.2f" % result["iops"]["rr"]
    print "        rw: %.2f" % result["iops"]["rw"]
    print "        sr: %d" % result["iops"]["sr"]
    print "        sw: %d" % result["iops"]["sw"]


if __name__ == "__main__":
    options = get_parser().parse_cmd()

    result = main(options)

    print_result(result, options)
