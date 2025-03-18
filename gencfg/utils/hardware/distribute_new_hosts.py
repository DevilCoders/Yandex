#!/usr/bin/env python
# coding=utf-8

import sys


def _distribute_ticket(ticket, segment, servers):
    # print "_distribute_ticket", ticket, segment, servers

    servers = ",".join(sorted(set(servers)))

    commands = ["./update.sh"]
    if segment == "rtc":
        commands.append("./utils/hardware/move_hosts.py -y -a move_hosts_to_reserve -s %s" % servers)
    else:
        if segment == "qloud":
            group = "ALL_QLOUD_HOSTS"
        elif segment == "yp":
            group = "ALL_YP_HOSTS"
        else:
            assert False

        commands.append("./utils/hardware/move_hosts.py -y -a move_hosts -g %s -s %s" % (group, servers))

    commands.append("./commit.sh -m 'new hosts to %s %s'" % (segment, ticket))

    sys.stdout.write(" && ".join(commands) + "\n\n")


def distribute():
    ticket = None
    segment = None
    servers = []
    known_dc = "iva myt sas vla man".split()

    for line in sys.stdin:
        if line.startswith("https://"):
            if ticket and segment:
                _distribute_ticket(ticket, segment, servers)

            line = line.split()
            ticket = line[0].split("/")[-1]
            segment = line[1] if len(line) > 1 else "rtc"
            servers = []
            continue

        line = line.replace(",", " ").split()
        if not line:
            continue

        if line[0] in known_dc:
            # this case:
            # sas	900916238	webface01ht.stat.yandex.net	E5-2650v2	128	0	8000
            servers.append(line[1])
        else:
            # assuming that's list of invnum or fqdn
            servers.extend(line)

    if ticket and segment:
        _distribute_ticket(ticket, segment, servers)


def main():
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option('--distribute', action="store_true")

    (options, args) = parser.parse_args()

    distribute()


if __name__ == "__main__":
    main()
