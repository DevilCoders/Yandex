import argparse
import datetime
import importlib

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--log', help='log table path')
    parser.add_argument('--output-path', help='output tables path')
    parser.add_argument('--proxy', help='YT proxy', default='hahn')
    parser.add_argument('--just-extract', action='store_true')
    parser.add_argument('--overwrite', action='store_true')
    parser.add_argument('--ttl', help='output table ttl in days', default=None)
    parser.add_argument('--tag', help='show tracks with this tag', default=None)
    parser.add_argument('--report', help='build visual report', action='store_true')
    args = parser.parse_args()

    expiration_time = None
    if args.ttl is not None:
        expiration_time = (datetime.datetime.now() + datetime.timedelta(days=args.ttl)).isoformat()

    lib = importlib.import_module(
        "cloud.blockstore.tools.analytics.find-perf-bottlenecks.lib")
    ytlib = importlib.import_module(
        "cloud.blockstore.tools.analytics.find-perf-bottlenecks.ytlib")

    if args.just_extract:
        ytlib.extract_traces(args.log, args.output_path, args.proxy, expiration_time)
    else:
        # ytlib.find_perf_bottlenecks(args.log, args.output_path, args.proxy, expiration_time, args.overwrite, tag=args.tag)
        result = ytlib.describe_slow_requests(args.log, args.output_path, args.proxy, expiration_time, args.overwrite, tag=args.tag)

        if args.report and result:
            report = lib.build_report(result.data())
            for description, report in report:
                with open(description + ".html", "w") as f:
                    f.write(report)
