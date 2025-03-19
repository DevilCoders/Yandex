#!/usr/bin/env python

import json
import socket


def gen_getter(itype, ctype, tier):
    try:
        with open('/tmp/.grains_conductor.cache') as f:
            grains = json.loads(''.join(f.readlines()))
    except Exception:
        grains = {}

    prj = "{{ prj }}"
    datacenter = grains.get('short_dc', 'nodc')
    if not datacenter:
        switch = grains.get('switch')
        if switch:
            datacenter = switch[:3]
        else:
            datacenter = 'nodc'
    res = socket.getfqdn() + ':11003@' + itype + \
          ' a_itype_' + itype + \
          ' a_prj_' + prj + \
          ' a_ctype_' + ctype + ' a_geo_' + datacenter + \
          ' a_tier_' + tier
    return res


def main():
    itypes = "{{ itypes }}".split(',')
    ctype = "{{ ctype }}"
    tier = 'replica'

    reports = [gen_getter(itype, ctype, tier) for itype in itypes]
    print('\n'.join(reports))


if __name__ == '__main__':
    main()
