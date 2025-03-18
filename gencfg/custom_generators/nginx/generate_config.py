#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

import json
import re

import gencfg
import core.argparse.types as argparse_types
from core.argparse.parser import ArgumentParserExt
from config import MAIN_DB_DIR


def get_parser():
    parser = ArgumentParserExt(description="Generate config for nginx (example in task RCPS-3046)")
    parser.add_argument("-t", "--template", type=str, required=True,
                        help="Obligatory. Template file name (from configs/nginx/templates)")
    parser.add_argument("-f", "--filler", type=str, required=True,
                        help="Obligatory. Filler file name (from configs/nginx/fillers)")
    parser.add_argument("-o", "--out-file", type=str, required=True,
                        help="Obligatory. Output file path")

    return parser


def normalize(options):
    options.template = os.path.join(MAIN_DB_DIR, "configs", "nginx", "templates", options.template)
    options.filler = os.path.join(MAIN_DB_DIR, "configs", "nginx", "fillers", options.filler)
    pass


def convert_gencfg_groups_to_nginx_backends(groups):
    all_instances = reduce(lambda x, y: x + y, map(lambda x: x.get_kinda_busy_instances(), groups), [])
    all_instances.sort(cmp=lambda x, y: cmp(x.name(), y.name()))
    return "\n".join(map(lambda x: "server %s:%s weight=%s;" % (x.host.name, x.port, int(x.power)), all_instances))


def convert_replacement_element(d):
    """
        Function gets config (json dict), which defines what to replace in template. Dict contains at least 3 fields:
            - key (name in template to replace)
            - value (replacement in some form)
            - type (method to convert value to result string)

        :param d (dict): input dict
        :return (str, str): pair of (key, value)
    """

    TYPES = ["str", "groups"]

    assert "key" in d and "value" in d and "type" in d
    assert d["type"] in TYPES, "Unknown type <%s>" % d["type"]

    if d["type"] == "str":
        return (str(d["key"]), str(d["value"]))
    elif d["type"] == "groups":
        groups = argparse_types.groups(str(d["value"]))
        return (str(d["key"]), convert_gencfg_groups_to_nginx_backends(groups))
    else:
        raise Exception("Unknown type <%s> while converting replacement element" % (d["type"]))


def modify_template(template_body, filler_dict):
    """
        Function replace all instances of {{somekey}} in template_body by text, found in filler_dict[somekey]

        :param template_body(str): content of nginx config template
        :param filler_dict(dict): dict with what we want to replace in template_body
    """

    # fill multiline
    p = re.compile("^([ \t]*){{(\w+)}}([ \t]*)$", flags=re.MULTILINE)
    while True:
        m = re.search(p, template_body)
        if m is None:
            break

        prefix = m.group(1)
        key = m.group(2)
        postfix = m.group(3)

        if key not in filler_dict:
            raise Exception("Key <%s> from template not found in filler dict" % (key))

        value = filler_dict[key]

        value = "\n".join(map(lambda x: "%s%s%s" % (prefix, x, postfix), value.split('\n')))
        template_body = re.sub(p, value, template_body, count=1)

    # fill singleline
    p = re.compile("{{(\w+)}}")
    while True:
        m = re.search(p, template_body)
        if m is None:
            break

        key = m.group(1)

        if key not in filler_dict:
            raise Exception("Key <%s> from template not found in filler dict" % (key))

        value = filler_dict[key]
        template_body = re.sub(p, value, template_body, count=1)

    return template_body


def main(options):
    template_body = open(options.template).read().strip()

    filler_json = json.loads(open(options.filler).read())
    filler_dict = dict()
    for elem in filler_json:
        k, v = convert_replacement_element(elem)
        assert k not in filler_dict, "Key <%s> found at least twice in config <%s>" % (options.filler)
        filler_dict[k] = v

    result = modify_template(template_body, filler_dict)

    with open(options.out_file, "w") as f:
        f.write(result)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
