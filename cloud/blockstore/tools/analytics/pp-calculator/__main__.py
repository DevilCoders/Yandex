import argparse
import json
import logging
import math
import sys

"""
    input:
        {
            "disk-id1": {
                "VolumeConfig": {
                    "StorageMediaKind": 1 (ssd) or 0 (hdd),
                    "Version": 123,
                    "BlockCount": 100500
                },
                "R-stat": {
                    "I-stat": [
                        [500, 100, 50KB/s],
                        [900, 200, 100KB/s],
                        [950, 300, 150KB/s],
                    ],
                    "B-stat": [
                        [500, 10, 500KB/s],
                        [900, 20, 1000KB/s],
                        [950, 30, 1500KB/s],
                    ]
                },
                "W-stat": {
                    "I-stat": [
                        [500, 100, 50KB/s],
                        [900, 200, 100KB/s],
                        [950, 300, 150KB/s],
                    ],
                    "B-stat": [
                        [500, 10, 500KB/s],
                        [900, 20, 1000KB/s],
                        [950, 30, 1500KB/s],
                    ]
                }
            },
            "disk-id2": {
                ...
            },
            ...
        }

    output:
        {
            "disk-id1": {
                "StorageMediaKind": 1 (ssd) or 0 (hdd),
                "Version": 123,
                "BlockCount": 100500,
                "PerformanceProfile": {
                    ...
                }
            },
            "disk-id2": {
                ...
            },
            ...
        }
"""

BANDWIDTH_LIMIT = 1 << 31
IOPS_LIMIT = 1 << 20

TYPE_STAT_DESCRS = [
    ("R-stat", "MaxReadIops", "MaxReadBandwidth", True),
    ("W-stat", "MaxWriteIops", "MaxWriteBandwidth", False),
]


def find_stat(stats, ppm):
    for x in stats:
        if x[0] == ppm:
            return (float(x[1]), float(x[2]))
    raise Exception("%s not found in stats" % ppm)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--volume-usage-stats', help='blockstore-volume-usage-stats tool output', required=True)

    parser.add_argument('--ssd-unit-read-iops', help='max read iops per ssd allocation unit', type=float, default=400)
    parser.add_argument('--ssd-unit-write-iops', help='max write iops per ssd allocation unit', type=float, default=1000)
    parser.add_argument('--ssd-unit-read-bandwidth', help='max read bandwidth (in MB/s) per ssd allocation unit', type=float, default=15)
    parser.add_argument('--ssd-unit-write-bandwidth', help='max write bandwidth (in MB/s) per ssd allocation unit', type=float, default=15)

    parser.add_argument('--ssd-max-read-iops', help='max read iops per ssd disk', type=float, default=12000)
    parser.add_argument('--ssd-max-write-iops', help='max write iops per ssd disk', type=float, default=40000)
    parser.add_argument('--ssd-max-read-bandwidth', help='max read bandwidth (in MB/s) per ssd disk', type=float, default=450)
    parser.add_argument('--ssd-max-write-bandwidth', help='max write bandwidth (in MB/s) per ssd disk', type=float, default=450)

    parser.add_argument('--hdd-unit-read-iops', help='max read iops per hdd allocation unit', type=float, default=100)
    parser.add_argument('--hdd-unit-write-iops', help='max write iops per hdd allocation unit', type=float, default=300)
    parser.add_argument('--hdd-unit-read-bandwidth', help='max read bandwidth (in MB/s) per hdd allocation unit', type=float, default=30)
    parser.add_argument('--hdd-unit-write-bandwidth', help='max write bandwidth (in MB/s) per hdd allocation unit', type=float, default=30)

    parser.add_argument('--hdd-max-read-iops', help='max read iops per hdd disk', type=float, default=300)
    parser.add_argument('--hdd-max-write-iops', help='max write iops per hdd disk', type=float, default=11000)
    parser.add_argument('--hdd-max-read-bandwidth', help='max read bandwidth (in MB/s) per hdd disk', type=float, default=240)
    parser.add_argument('--hdd-max-write-bandwidth', help='max write bandwidth (in MB/s) per hdd disk', type=float, default=240)

    parser.add_argument('--nonrepl-unit-read-iops', help='max read iops per nonrepl allocation unit', type=float, default=28000)
    parser.add_argument('--nonrepl-unit-write-iops', help='max write iops per nonrepl allocation unit', type=float, default=5600)
    parser.add_argument('--nonrepl-unit-read-bandwidth', help='max read bandwidth (in MB/s) per nonrepl allocation unit', type=float, default=110)
    parser.add_argument('--nonrepl-unit-write-bandwidth', help='max write bandwidth (in MB/s) per nonrepl allocation unit', type=float, default=82)

    parser.add_argument('--nonrepl-max-read-iops', help='max read iops per nonrepl disk', type=float, default=62000)
    parser.add_argument('--nonrepl-max-write-iops', help='max write iops per nonrepl disk', type=float, default=62000)
    parser.add_argument('--nonrepl-max-read-bandwidth', help='max read bandwidth (in MB/s) per nonrepl disk', type=float, default=1024)
    parser.add_argument('--nonrepl-max-write-bandwidth', help='max write bandwidth (in MB/s) per nonrepl disk', type=float, default=1024)

    parser.add_argument('--ppm', help='the usage ppm for real usage calculation', type=int, default=999)
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')

    parser.add_argument('--allow-no-stat', help='allows no R-stat/W-stat in input', action='store_true')
    parser.add_argument('--force-limits', help='forces limit calculation even if volume stats were not found', action='store_true')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.ERROR

    logging.basicConfig(stream=sys.stderr, level=log_level, format="[%(levelname)s] [%(asctime)s] %(message)s")

    with open(args.volume_usage_stats) as f:
        volume_usage_stats = json.load(f)

    result = {}

    for disk_id, stats in volume_usage_stats.iteritems():
        vc = stats["VolumeConfig"]
        block_count = 0
        for partition_config in vc["Partitions"]:
            block_count += int(partition_config["BlockCount"])
        sz = block_count * vc.get("BlockSize", 4096)
        assert sz > 0

        media_kind = vc["StorageMediaKind"]

        is_ssd = media_kind == 1
        is_nonrepl = media_kind == 4

        MB = 2 ** 20
        GB = 2 ** 30
        au_size = [0, 32, 0, 256, 93][media_kind] * GB
        au_count = math.ceil(float(sz) / au_size)

        performance_profile = {
            "BurstPercentage": 10,
            "MaxPostponedWeight": 128 * 1024 * 1024,
            "ThrottlingEnabled": True
        }

        for type_stat_descr in TYPE_STAT_DESCRS:
            # calculating suggested params
            if type_stat_descr[3]:
                if is_ssd:
                    iops_per_au = args.ssd_unit_read_iops
                    bandwidth_per_au = args.ssd_unit_read_bandwidth * MB
                    iops_per_disk = args.ssd_max_read_iops
                    bandwidth_per_disk = args.ssd_max_read_bandwidth * MB
                elif is_nonrepl:
                    iops_per_au = args.nonrepl_unit_read_iops
                    bandwidth_per_au = args.nonrepl_unit_read_bandwidth * MB
                    iops_per_disk = args.nonrepl_max_read_iops
                    bandwidth_per_disk = args.nonrepl_max_read_bandwidth * MB
                else:
                    iops_per_au = args.hdd_unit_read_iops
                    bandwidth_per_au = args.hdd_unit_read_bandwidth * MB
                    iops_per_disk = args.hdd_max_read_iops
                    bandwidth_per_disk = args.hdd_max_read_bandwidth * MB
            else:
                if is_ssd:
                    iops_per_au = args.ssd_unit_write_iops
                    bandwidth_per_au = args.ssd_unit_write_bandwidth * MB
                    iops_per_disk = args.ssd_max_write_iops
                    bandwidth_per_disk = args.ssd_max_write_bandwidth * MB
                elif is_nonrepl:
                    iops_per_au = args.nonrepl_unit_write_iops
                    bandwidth_per_au = args.nonrepl_unit_write_bandwidth * MB
                    iops_per_disk = args.nonrepl_max_write_iops
                    bandwidth_per_disk = args.nonrepl_max_write_bandwidth * MB
                else:
                    iops_per_au = args.hdd_unit_write_iops
                    bandwidth_per_au = args.hdd_unit_write_bandwidth * MB
                    iops_per_disk = args.hdd_max_write_iops
                    bandwidth_per_disk = args.hdd_max_write_bandwidth * MB

            suggested_iops = min(iops_per_disk, au_count * iops_per_au)
            suggested_bandwidth = min(bandwidth_per_disk, au_count * bandwidth_per_au)

            type_stat = stats.get(type_stat_descr[0])
            if not args.allow_no_stat and type_stat is None:
                raise Exception("no %s stat" % type_stat_descr[0])

            if type_stat is not None:
                # calculating params according to real usage
                i_stat = find_stat(type_stat["I-stat"], args.ppm)
                b_stat = find_stat(type_stat["B-stat"], args.ppm)

                b_i = i_stat[0]
                i = i_stat[1]
                b = b_stat[0]
                i_b = b_stat[1]

                i_coeff = max(i / suggested_iops + b_i / suggested_bandwidth, 1.)
                b_coeff = max(i_b / suggested_iops + b / suggested_bandwidth, 1.)
                coeff = max(i_coeff, b_coeff)

                max_iops = int(math.ceil(i_coeff * suggested_iops))
                max_bandwidth = int(math.ceil(b_coeff * suggested_bandwidth))
            else:
                coeff = 1

                max_iops = int(suggested_iops)
                max_bandwidth = int(suggested_bandwidth)

            if max_iops > IOPS_LIMIT:
                print >> sys.stderr, "%s: iops exceeds limit: %s > %s"  \
                    % (disk_id, max_iops, IOPS_LIMIT)
            if max_bandwidth > BANDWIDTH_LIMIT:
                print >> sys.stderr, "%s: bandwidth exceeds limit: %s MB/s > %s MB/s"   \
                    % (disk_id, math.ceil(max_bandwidth / MB), math.ceil(BANDWIDTH_LIMIT / MB))

            # we can safely build new limits if either perf profile is not set
            # up for volume or we have usage stats for this volume
            if "PerformanceProfileMaxReadIops" not in vc or type_stat is not None or args.force_limits:
                performance_profile[type_stat_descr[1]] =   \
                    min(max_iops, IOPS_LIMIT)
                performance_profile[type_stat_descr[2]] =   \
                    min(max_bandwidth, BANDWIDTH_LIMIT)

            if coeff > 1:
                print >> sys.stderr, "\t".join([
                    disk_id,
                    vc["CloudId"],
                    ["", "SSD", "", "HDD", "Nonrepl"][media_kind],
                    type_stat_descr[0],
                    "SetIops=%s" % max_iops,
                    "SuggestedIops=%s" % int(math.ceil(suggested_iops)),
                    "ActualIops=%s" % max(int(math.ceil(i)), 1),
                    "SetBandwidth=%sMB/s" % math.ceil(max_bandwidth / MB),
                    "SuggestedBandwidth=%sMB/s" % math.ceil(suggested_bandwidth / MB),
                    "ActualBandwidth=%sMB/s" % math.ceil(b / MB),
                    "ShouldBeResizedTo=%sGB" % (math.ceil(coeff * au_count) * (au_size / GB)),
                ])

        r = result[disk_id] = {}
        r["BlockCount"] = block_count
        r["Version"] = vc["Version"]
        r["StorageMediaKind"] = vc["StorageMediaKind"]
        r["PerformanceProfile"] = performance_profile

    json.dump(result, sys.stdout, indent=4)


if __name__ == '__main__':
    sys.exit(main())
