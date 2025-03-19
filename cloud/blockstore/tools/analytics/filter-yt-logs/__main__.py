import argparse
import datetime
import importlib


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--src', help='source YT table')
    parser.add_argument('--dst', help='destination YT table')
    parser.add_argument('--filter-type', help='(logs|trace|qemu)')
    parser.add_argument('--proxy', help='YT proxy', default='hahn')
    parser.add_argument('--pool', help='YT pool', default=None)
    parser.add_argument('--ttl', help='output table ttl in days', default=None)
    args = parser.parse_args()

    expiration_time = None
    if args.ttl is not None:
        expiration_time = (datetime.datetime.now() + datetime.timedelta(days=args.ttl)).isoformat()

    module = importlib.import_module("cloud.blockstore.tools.analytics.filter-yt-logs.lib")
    filter_type_to_func = {
        'logs': module.filter_nbs_logs,
        'trace': module.filter_nbs_traces,
        'qemu': module.filter_qemu_logs
    }
    filter_type_to_func[args.filter_type](args.src, args.dst, args.proxy, expiration_time, args.pool)
