import subprocess
import re

groups = [
    2181038796,
    2181038821,
    2181038822,
    2181038858,
    2181038876,
    2181038877,
    2181038878,
    2181038879,
    2181038893,
    2181038894,
    2181038908,
    2181038909,
    2181038911,
    2181038926,
    2181038927,
    2181038940,
    2181038943,
    2181038956,
    2181038958,
    2181039006,
    2181039024,
    2181039060
]

pattern = 'https://ydb.bastion.cloud.yandex-team.ru/sas09-ct9-1.cloud.yandex.net:8766/tablets/app?TabletID=72057594037968897&page=Groups&group_id={}'
cookies = 'Insert your cookies here'

tablets = []
for g in groups:
    r = subprocess.check_output(
        [
            'curl',
            pattern.format(g),
            '-H',
            'Connection: keep-alive',
            '-H',
            'Cache-Control: max-age=0',
            '-H',
            'Upgrade-Insecure-Requests: 1',
            '-H',
            'User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 YaBrowser/20.3.0.2220 Yowser/2.5 Safari/537.36',
            '-H',
            'Sec-Fetch-Dest: document',
            '-H',
            'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9',
            '-H',
            'Sec-Fetch-Site: same-origin',
            '-H',
            'Sec-Fetch-Mode: navigate',
            '-H',
            'Sec-Fetch-User: ?1',
            '-H',
            'Accept-Language: ru,en;q=0.9,la;q=0.8,fr;q=0.7,es;q=0.6',
            '-H',
            'Cookie: {}'.format(cookies),
            '--compressed'
        ],
    )

    for t in re.findall('TabletID=([0-9]+)', r):
        tablets.append(t)

f = open('tablets_from_groups.res', 'w')
for t in tablets:
    f.write(t + '\n')
