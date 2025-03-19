import argparse
import numpy as np
import re
from collections import namedtuple as namedtuple
from collections import Counter as Counter

RequestData = namedtuple('RequestData', 'type offset bytes')

def parse(logfile, block_size, volume_size, request_type):
    requests = []
    pattern = re.compile("blk_co_p(read|write)v blk .* bs .* offset (.*) bytes (.*) flags (.*)")
    for i, line in enumerate(open(logfile)):
        for match in re.finditer(pattern, line):
            requests.append(
                    RequestData(
                        type = match.groups()[0],
                        offset = int(match.groups()[1]),
                        bytes = int(match.groups()[2])))

    block_lengths = Counter()
    for r in requests:
        if r.type == request_type:
            block_lengths[r.bytes // block_size] += 1;

    print("Length,Count");
    for key in sorted(block_lengths.keys()):
        print("{0},{1}".format(key, block_lengths[key]))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('logfile', help='qemu io trace log file')
    parser.add_argument('--block_size', help='block size of the volume (4096 by default)', type=int, default=4096)
    parser.add_argument('--volume_size', help='size of the volume)', type=int)
    parser.add_argument('--request_type', help='type of request (read|write)', type=str)

    args = parser.parse_args()
    parse(args.logfile, args.block_size, args.volume_size, args.request_type)
