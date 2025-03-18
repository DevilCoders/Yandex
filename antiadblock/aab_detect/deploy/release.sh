#!/bin/bash
# A simple script to deploy our files to s3
# Wiki link:
# https://wiki.yandex-team.ru/taxi/ml/youngfighter/mds/

aws --endpoint-url=http://s3.mds.yandex.net s3 cp dist/lib.browser.min.js s3://aab-pub/lib.browser.min.js
aws --endpoint-url=http://s3.mds.yandex.net s3 cp dist/with_cm.lib.browser.min.js s3://aab-pub/with_cm.lib.browser.min.js
aws --endpoint-url=http://s3.mds.yandex.net s3 cp dist/without_cm.lib.browser.min.js s3://aab-pub/without_cm.lib.browser.min.js
aws --endpoint-url=http://s3.mds.yandex.net s3 cp dist/minimal.lib.browser.min.js s3://aab-pub/minimal.lib.browser.min.js
