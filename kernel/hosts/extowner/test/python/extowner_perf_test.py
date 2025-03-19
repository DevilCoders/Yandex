import yatest.common


def test_extowner(metrics):
    repeat_number = "300"

    cmd = [
        yatest.common.build_path('kernel/hosts/extowner/test/test'),
        '-a', yatest.common.source_path('yweb/urlrules/areas.lst'),
        '-u', 'urls/urls.txt',
        '-r', repeat_number
    ]

    res = yatest.common.execute(cmd)
    metrics.set("extowner_utime", res.metrics['utime'])
