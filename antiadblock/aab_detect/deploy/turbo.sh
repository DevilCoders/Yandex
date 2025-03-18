
#!/bin/bash
# A simple script to deploy our files to s3
# Wiki link:
# https://wiki.yandex-team.ru/taxi/ml/youngfighter/mds/

aws --endpoint-url=http://s3.mds.yandex.net s3 cp dist/inject.min.js s3://aab-pub/turbo.lib.browser.min.js