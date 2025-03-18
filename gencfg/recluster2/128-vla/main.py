import sys
sys.path.append('../..')

from core.db import CURDB


def main():
    vla_yt_rtc = CURDB.groups.get_group('VLA_YT_RTC')
    for grp in vla_yt_rtc.slaves:
        print grp.card.name
    # print 'total', len(vla_yt_rtc.slaves), 'groups'


if __name__ == '__main__':
    main()
