from collections import defaultdict, Counter


def load_groups(file_name):
    groups = defaultdict(Counter)
    with open(file_name) as f:
        for line in f.readlines():
            host, group, count = line.split()
            groups[group][host] += int(count)
    return groups


if __name__ == '__main__':
    import sys
    groups = load_groups(sys.argv[1])
    print groups['MAN_WEB_TIER1_JUPITER_BASE'].most_common(5)
