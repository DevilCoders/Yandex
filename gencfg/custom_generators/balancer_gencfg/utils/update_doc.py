#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import requests
from argparse import ArgumentParser

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

ACTIONS = ["show", "update"]


def index_table(content):
    result = "#|\n"

    for helper in content:
        name = helper[helper.find("===") + 3:]
        name = name[:name.find("===")]
        name = name.strip()
        desc = helper[helper.find("===") + 3:helper.find("<#")]
        desc = desc[desc.find("===") + 3:]
        desc = desc.strip()
        result += "|| ((#%s %s)) | %s ||\n" % (name.lower(), name, desc)

    result += "|#\n"
    return result


def parse_cmd():
    parser = ArgumentParser(description="Update balancer documentation (write to wiki)")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute")
    parser.add_argument("-o", "--oauth", type=str,
                        help="OAuth token")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def save_page(url, title, body, oauth):
    URL = "https://wiki-api.yandex-team.ru/_api/frontend/%s" % url
    DATA = {"title": title, "body": body}
    HEADERS = {'Authorization': 'OAuth %s' % oauth, 'Content-Type': 'application/json'}
    r = requests.post(URL, headers=HEADERS, json=DATA)
    print r
    if r.status_code != 200:
        raise Exception("Can't save page %s" % url)


def filter_by_type(what, expr):
    name = what[what.find("===") + 3:]
    name = name[:name.find("===")]
    name = name.strip()
    return name.startswith(expr)


def main(options):
    from src import modules, macroses
    from src.optionschecker import HELPERS
    doc_data = map(lambda x: str(x), HELPERS)
    doc_data.sort()

    # get modules content
    modules = [desc for desc in doc_data if filter_by_type(desc, 'Modules')]
    modules_content = index_table(modules)
    modules_content += '\n'.join(modules)

    # get macroses content
    macroses = [desc for desc in doc_data if filter_by_type(desc, 'Macroses')]
    macroses_content = index_table(macroses)
    macroses_content += '\n'.join(macroses)

    if options.action == "show":
        print modules_content
        print macroses_content
    elif options.action == "update":
        if options.oauth:
            save_page('/jandekspoisk/sepe/balancer/balancerconfig/modules', 'Модули', modules_content, options.oauth)
            save_page('/jandekspoisk/sepe/balancer/balancerconfig/macroses', 'Макросы', macroses_content, options.oauth)
        else:
            raise Exception("Need to specify token with -o option")
    else:
        raise Exception("Unkown action %s" % options.action)


if __name__ == '__main__':
    options = parse_cmd()

    main(options)
