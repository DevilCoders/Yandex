#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

'''
USAGE EXAMPLE:
./../../utils/common/dump_hostsdata.py -i dc,invnum,name,model,memory,ssd,disk,switch -s sas2-7037.search.yandex.net,vla1-4334.search.yandex.net,iva1-pgaas-0006.db.yandex.net | python fix_dump_hostsdata_output.py
'''

import sys
import re
import requests
import warnings

BOT_BASE_API = 'https://bot.yandex-team.ru/api/'


def get_dump_hostdata_res():
    return sys.stdin.read()


def get_info_from_bot_api(host_name=None, invnum=None):
    assert host_name or invnum
    if invnum:
        key = "inv"
        val = invnum
    else:
        key = "name"
        val = host_name

    dump_host_api = '{0}consistof.php?{1}={2}'.format(BOT_BASE_API, key, val)

    response = None
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        # warnings.warn('deprecated', requests.packages.urllib3.exceptions.InsecurePlatformWarning)
        response = requests.get(dump_host_api, verify=False)

    return response.text.encode('utf-8') if response else None


BASE_MULTS = {
    2: {
        "MB": 1,
        "GB": 1024,
        "TB": 1024 ** 2,
    },
    10: {
        "MB": float(1000) ** 2 / (1024 ** 2),
        "GB": float(1000) ** 3 / (1024 ** 2),
        "TB": float(1000) ** 4 / (1024 ** 2),
    }
}


def calc_reg_exp(host_info, information_unit, base=2):
    host_value = None

    exp = r'\b\d+' + information_unit
    regex = re.compile(exp)

    value_list = [value for value in host_info if regex.search(value)]

    if value_list:
        host_value = int(value_list[0].partition(information_unit)[0])

        if information_unit not in ['GB', 'TB']:
            information_unit = 'MB'

        host_value *= BASE_MULTS[base][information_unit]

    return host_value


def parse_special_cases(host_info):
    model = host_info[3]
    model_specs = host_info[4]

    standard_sizes = ["8", "16", "32", "64"]

    if model == 'SAMSUNG':
        if "G" in model_specs:
            # 16G2RX4PC3L
            size = model_specs.split("G")[0]
        elif model_specs == "PC4" and " ".join(host_info[4:]).startswith("PC4 2933Y RA2 12 DC1 S N"):
            size = "64"
        else:
            return

        if not size in standard_sizes:
            return

        return int(size) * 1024

    elif model == 'NANYA':
        if model_specs == 'NT16GC72C4NB0NL':
            return 16 * 1024

    elif model == "KINGSTON":
        # Оперативная память KINGSTON KVR13LR9D4 16 S N
        # Оперативная память KINGSTON KVR1333D3D4R9S 8G S N
        # Оперативная память KINGSTON KVR13LR9D4 8HC S N
        size = re.search("[0-9]+", host_info[5])
        size = size.group() if size else None
        if not size in standard_sizes:
            return None

        return int(size) * 1024


def get_information_capacity_mb(invnum, host_info, base=2):
    for information_unit in ['MB', 'GB', 'TB']:
        value = calc_reg_exp(host_info, information_unit, base=base)
        if value:
            return value

    special_parsed_value = parse_special_cases(host_info)

    if special_parsed_value:
        return special_parsed_value

    raise Exception('Can not parse %s component: %s' % (invnum, ' '.join(host_info)))


def parse_bot_host_info(invnum, bot_host_info, fix_memory=True):
    platform = None
    ram = 0
    ssd = 0
    nvme = 0
    hdd = 0

    bot_host_info_list = bot_host_info.splitlines()

    model = bot_host_info_list[1].split()
    if len(model) > 3:
        temp = model[3].split("/")
        if len(temp) > 2:
            platform = temp[2]

    nvme_support = "u.2" in re.sub(r'(<br>)|[\-\!\$\(\)\*\+\<\>\?\[\\/\\\]^{|}\:]', r' ', bot_host_info_list[1]).lower()

    for host_bot_info_line in bot_host_info_list[2::]:
        host_info = re.sub(r'(<br>)|[\-\!\$\(\)\*\+\.\<\>\?\[\\/\\\]^{|}\:]', r' ', host_bot_info_line)
        host_info = host_info.split()

        if fix_memory and host_info[1] == 'Оперативная' and host_info[2] == 'память':
            ram += get_information_capacity_mb(invnum, host_info)
        elif host_info[2] == 'SSD':
            if "/U.2/" in host_bot_info_line:
                nvme += get_information_capacity_mb(invnum, host_info, base=10)
            else:
                ssd += get_information_capacity_mb(invnum, host_info, base=10)
        elif host_info[1] == 'HDD':
            if 'SSD' in host_info:
                if "/U.2/" in host_bot_info_line:
                    nvme += get_information_capacity_mb(invnum, host_info, base=10)
                else:
                    ssd += get_information_capacity_mb(invnum, host_info, base=10)
            else:
                hdd += get_information_capacity_mb(invnum, host_info, base=10)

    return platform, ram / 1024, ssd / 1024, nvme / 1024, hdd / 1024, nvme_support


def fix_dump_with_bot_api(hostdata_dump, separate_nvme=False, fix_memory=True, nvme_support_check=False):
    fixed_hostdata_dump = []
    for host in hostdata_dump.splitlines():
        host_info_list = host.split("\t")

        invnum = host_info_list[1]
        response = get_info_from_bot_api(invnum=invnum)

        if response:
            bot_host_info = response

            platform, host_memory, host_ssd, host_nvme, host_disk, nvme_support = parse_bot_host_info(invnum,
                                                                                                      bot_host_info,
                                                                                                      fix_memory=fix_memory)

            if nvme_support_check and not nvme_support:
                continue

            if not separate_nvme:
                host_ssd += host_nvme

            if fix_memory:
                host_info_list[4] = int(host_memory)
            host_info_list[5] = int(host_ssd)
            host_info_list[6] = int(host_disk)
            host_info_list.append(platform)

            if separate_nvme:
                host_info_list = host_info_list[:7]
                host_info_list.append(int(host_nvme))

        fixed_hostdata_dump.append([str(info) for info in host_info_list])

    return fixed_hostdata_dump


def print_fixed_dump(fixed_hostdata_dump):
    for host_dump in fixed_hostdata_dump:
        host_str_to_print = ''
        for host_value in host_dump:
            host_str_to_print += host_value + '\t'
        print host_str_to_print


def main():
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("--nvme", action="store_true", help="Separate NVME from SSD. Will change format")
    parser.add_option("--skip-memory", action="store_true", help="Separate NVME from SSD. Will change format")

    parser.add_option("--support-nvme", action="store_true", help="Leave only hosts what support nvme")
    (options, args) = parser.parse_args()

    hostdata_dump = get_dump_hostdata_res()
    fixed_hostdata_dump = fix_dump_with_bot_api(
        hostdata_dump,
        separate_nvme=options.nvme,
        fix_memory=not options.skip_memory,
        nvme_support_check=options.support_nvme,
    )
    print_fixed_dump(fixed_hostdata_dump)


if __name__ == '__main__':
    main()
