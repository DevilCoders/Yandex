import argparse
import sys

from infra.yp_quota_distributor.lib.hosts_quota import (
    calculate_and_format_quota_from_hosts_file,
    calculate_and_format_quota_from_hostsdata
)


def parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    g = parser.add_mutually_exclusive_group()
    g.add_argument('--hosts-file', help='File with invnums and fqdns', type=str)
    g.add_argument('--dump-hostsdata-input', help='Take data from stdin in dump_hostsdata.sh format',
                   action="store_true")

    parser.add_argument('--discount-mode', help='Discount mode')
    parser.add_argument('--no-discount', help='Don\'t apply cpu discount',
                        action="store_true")
    parser.add_argument('--gencfg', help='gencfg mode without infra tax', action="store_true")
    parser.add_argument('--json-format', help='json format', action="store_true")

    return parser.parse_args()


def load_hostsdata_from_stdin():
    hostsdata = {}

    for line in sys.stdin:
        line = line.strip().split('\t')
        memory = line[4]
        if memory == "504":  # workaround for old gencfg bug
            memory = "512"

        gpu_models = list(set(line[10].split(','))) if len(line) > 10 else []

        if len(gpu_models) > 1:
            raise ValueError("Not unique gpu model for line {}".format(line))
        gpu_model = None
        if len(gpu_models) > 0 and gpu_models[0] != '':
            gpu_model = gpu_models[0]

        hostsdata[line[2]] = {
            'dc': line[0],
            'inv': line[1],
            'cpu_model': line[3],
            'memory': memory,
            'ssd': int(line[5]) / 1024,  # GB to TB
            'disk': int(line[6]) / 1024,  # GB to TB
            'gpu_model': gpu_model,
            'gpu_count': line[11] if len(line) > 11 else 0,
        }

    return hostsdata


def main():
    args = parse_args()
    use_format = "json" if args.json_format else "text"

    if args.discount_mode:
        discount_mode = args.discount_mode
    elif args.no_discount:
        discount_mode = "infra_only"
    elif args.gencfg:
        discount_mode = "gencfg"
    else:
        discount_mode = "full"

    if args.hosts_file:
        file_name = args.hosts_file
        data = [item.strip() for item in open(file_name, 'r').readlines()]
        text, _ = calculate_and_format_quota_from_hosts_file(
            data,
            abc_info='TEST',
            abc_segment='TEST',
            console_flag=True,
            discount_mode=discount_mode,
            use_format=use_format,
            ignore_banned_lists=True
        )
        print(text)
    elif args.dump_hostsdata_input:
        hostsdata = load_hostsdata_from_stdin()
        hosts = hostsdata.keys()

        text, quotas = calculate_and_format_quota_from_hostsdata(
            hosts,
            hostsdata,
            abc_info='TEST',
            abc_segment='TEST',
            discount_mode=discount_mode,
            use_format=use_format,
            ignore_banned_lists=True
        )
        print(text)
        if quotas.get_errors():
            sys.stderr.write("ERRORS:\n")
            sys.stderr.write('\n'.join(quotas.get_errors()) + "\n")
    else:
        sys.stderr.write("Specify file\n")
        exit(1)
