from random import randint

from django.core.management.base import BaseCommand

from antirobot.cbb.cbb_django.cbb.models import Group, BLOCK_VERSIONS
from antirobot.cbb.cbb_django.cbb.library import db
from ipaddr import IPAddress

from urllib import urlencode
from optparse import make_option

BlockIPV4 = BLOCK_VERSIONS[4]

with db.main.use_slave():
    groups_query = Group.query.filter_by(is_active=True)

    groups_full = groups_query.all()
    groups = {}
    groups[0] = groups_query.filter_by(default_type="0").all()
    groups[1] = groups_query.filter_by(default_type="1").all()
    groups[2] = groups_query.filter_by(default_type="2").all()
    groups[3] = groups_query.filter_by(default_type="3").all()


def generate_ipv4():
    return ".".join(map(str, [randint(0, 255), randint(0, 255), randint(0, 255), randint(0, 255)]))


def generate_ipv4_range():
    ip1 = randint(16777216, 4294967295)  # 1.0.0.0 - 255.255.255.255
    ip2 = ip1 + randint(0, 255)
    return (str(IPAddress(ip1)), str(IPAddress(ip2)))


def query(handle, query):
    return handle + ".pl?%s" % urlencode(query)


def check_flag():
    for group in groups_full:
        print(query("check_flag", {"flag": group.id}))


def get_range():
    for rng in BlockIPV4.query.all():
        for (key, value) in (("with_expire", "yes"), ("with_except", "yes"), ("with_format", "range_dst")):
            print(query("get_range", {"range_src": rng.get_rng_start(), "range_dst": rng.get_rng_end(), key: value}))

            print(query("get_range", {"range_src": rng.get_rng_start(), "range_dst": rng.get_rng_end(), "flag": rng.group_id, key: value}))

            print(query("get_range", {"range_src": str(IPAddress(rng.rng_start - randint(1, 100))), "range_dst": str(IPAddress(rng.rng_start + randint(1, 100))), key: value}))

            print(query("get_range", {"range_src": str(IPAddress(rng.rng_start - randint(1, 100))), "range_dst": str(IPAddress(rng.rng_start + randint(1, 100))), "flag": rng.group_id, key: value}))


def get_flag():
    for group in groups_full:
        print(query("get_range", {"flag": group.id}))


def get_netblock():
    for group in groups[2]:
        print(query("get_netblock", {"flag": group.id}))


def get_netexcept():
    for group in groups[2]:
        print(query("get_netexcept", {"flag": group.id}))


def set_range_0():
    # ranges
    for group in groups[0]:
        (rng_start, rng_end) = generate_ipv4_range()
        print(query("set_range", {"flag": group.id, "range_src": rng_start, "range_dst": rng_end, "operation": "add"}))
        print(query("set_range", {"flag": group.id, "range_src": rng_start, "range_dst": rng_end, "operation": "del"}))


def set_range_1():
    # single
    for group in groups[1]:
        rng_start = rng_end = generate_ipv4()
        print(query("set_range", {"flag": group.id, "range_src": rng_start, "range_dst": rng_end, "operation": "add"}))
        print(query("set_range", {"flag": group.id, "range_src": rng_start, "range_dst": rng_end, "operation": "del"}))


def set_range_2():
    # cidr
    for group in groups[2]:
        (rng_start, rng_end) = generate_ipv4_range()
        net_mask = randint(20, 31)
        except_net_mask = randint(net_mask+1, 32)

        # add netblock
        print(query("set_netblock", {"flag": group.id, "net_ip": rng_start, "net_mask": net_mask, "operation": "add"}))
        # add exception to this netblock
        print(query("set_netexcept", {"flag": group.id, "net_ip": rng_start, "net_mask": net_mask, "except_ip": rng_start, "except_mask": except_net_mask, "operation": "add"}))
        # remove exception
        print(query("set_netexcept", {"flag": group.id, "net_ip": rng_start, "net_mask": net_mask, "except_ip": rng_start, "except_mask": except_net_mask, "operation": "del"}))
        # remove netblock
        print(query("set_netblock", {"flag": group.id, "net_ip": rng_start, "net_mask": net_mask, "operation": "del"}))


def generate_get_queries():
    get_range()
    get_flag()
    check_flag()
    get_netblock()
    get_netexcept()


def generate_set_queries():
    set_range_0()
    set_range_1()
    set_range_2()


class Command(BaseCommand):
    help = "Generates queries for API handles (mostly IPV4)."

    option_list = BaseCommand.option_list + (
        make_option("--get", action="store_true", dest="get", default=False, help="Generates possible get queries on existing DB."),
        make_option("--set", action="store_true", dest="set", default=False, help="Generates partly random set queries."),
    )

    @db.main.use_slave()
    def handle(self, *args, **options):
        if options["get"]:
            generate_get_queries()
        if options["set"]:
            for i in range(100):
                generate_set_queries()
