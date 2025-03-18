#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from argparse import ArgumentParser

import gencfg
from core.hosts import Host, get_host_fields_info
import core.argparse.types as argparse_types
from core.db import CURDB


class EReportTypes(object):
    PLAIN = 'plain'
    JSON = 'json'
    DETAILED = 'detailed'
    ALL = [PLAIN, JSON, DETAILED]


def parse_cmd():
    parser = ArgumentParser(description="Script to dump hardware_data hosts_data")

    parser.add_argument("-s", "--hosts", dest="hosts", type=argparse_types.hosts, default=None,
                        help="Optional. Comma-separated list of hosts")
    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.xgroups, default=None,
                        help="Optional. Comma-separated list of groups")
    parser.add_argument("-i", "--fields", dest="fields", type=str, default="all",
                        help="Optional. List of fields to show: %s ('all' to show all)" % ','.join(
                            map(lambda x: x.name, get_host_fields_info())))
    parser.add_argument("-f", "--host-filter", dest="filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Host filter")
    parser.add_argument("--group-filter", dest="group_filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Group filter (check if any of host group satisfy filter)")
    parser.add_argument("--iv", dest="ignore_virtual", action="store_true", default=False,
                        help="Optional. Ignore virtual machines")
    parser.add_argument("--ing", dest="ignore_nonsearch_groups", action="store_true", default=False,
                        help="Optional. Ignore machines, which belongs to non-search groups")
    parser.add_argument("-r", "--report-type", type=str, default=EReportTypes.PLAIN,
                        choices=EReportTypes.ALL,
                        help="Report type. Show report type")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    if options.fields == 'all':
        options.fields = map(lambda x: x.name, get_host_fields_info())
    else:
        options.fields = options.fields.split(',')
    if options.hosts is None:
        if options.groups is None:
            options.hosts = argparse_types.hosts("ALL")
        else:
            options.hosts = list(set(sum(map(lambda x: x.getHosts(), options.groups), [])))

    return options


def report_host(options, host):
    if options.report_type == EReportTypes.PLAIN:
        printed_fields = []
        for field_name in options.fields:
            field_value = getattr(host, field_name)
            if isinstance(field_value, list):
                field_value = ','.join(field_value)
            elif isinstance(field_value, (int, float, bool, dict)):
                field_value = str(field_value)
            printed_fields.append(field_value)
        return '\t'.join(printed_fields)
    elif options.report_type == EReportTypes.JSON:
        return "%s" % host.save_to_json_string()
    elif options.report_type == EReportTypes.DETAILED:
        fields = map(lambda x: "   %s => %s" % (x, getattr(host, x)), options.fields)
        return "Host %s:\n%s" % (host.name, "\n".join(fields))
    else:
        raise Exception("Unkown report type <%s>" % options.report_type)


def main(options, db=CURDB):
    hosts = options.hosts

    if options.filter:
        hosts = filter(lambda x: options.filter(x), hosts)
    if options.ignore_virtual:
        hosts = filter(lambda x: x.is_vm_guest() == False, hosts)

    if options.group_filter:
        def has_filtered_group(host):
            for group in db.groups.get_host_groups(host):
                if group.card.properties.fake_group:
                    continue
                if group.card.properties.background_group:
                    continue
                if options.group_filter(group):
                    return True
            return False

        hosts = filter(lambda x: has_filtered_group(x), hosts)

    if options.ignore_nonsearch_groups:
        def has_nonsearch_group(host):
            return len(filter(lambda x: x.card.properties.nonsearch == True, db.groups.get_host_groups(host))) > 0

        hosts = filter(lambda x: not has_nonsearch_group(x), hosts)

    return hosts


if __name__ == '__main__':
    options = parse_cmd()

    hosts = main(options)

    for host in hosts:
        print report_host(options, host)
