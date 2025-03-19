#!/usr/bin/env python3

from yc_monitoring import JugglerPassiveCheck

VF_COUNT_FILE = '/sys/class/net/eth0/device/sriov_numvfs'
EXPECTED_VF_COUNT = 64


def main():
    check = JugglerPassiveCheck('vf_count')
    try:
        with open(VF_COUNT_FILE) as fd:
            contents = fd.read().strip()
            if not contents.isnumeric():
                check.crit('{} should contain a numeric value'.format(VF_COUNT_FILE))
            if int(contents) != EXPECTED_VF_COUNT:
                check.crit("Expected '{}', got '{}' in file {}".format(EXPECTED_VF_COUNT, contents, VF_COUNT_FILE))
    except Exception as e:
        check.crit('An error occurred during check: ({}): {}'.format(e.__class__.__name__, e))
    print(check)


if __name__ == '__main__':
    main()
