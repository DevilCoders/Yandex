import json
import os
import random

import mock
from cloud.billing.utils.scripts.table_from_yt_to_s3 import table_from_yt_to_s3


def generate_id():
    alph = "0123456789qwertyuiopasdfghjklzxcvbnm"
    s = ''
    for i in range(16):
        s += random.choice(alph)
    return s


@mock.patch("boto3.session.Session", autospec=True)
@mock.patch("time.time", autospec=True)
def test_full_case(mocked_time, mocked_session):
    import yt.wrapper as yt
    yt_proxy = os.environ["YT_PROXY"]
    yt.config['proxy']['url'] = yt_proxy
    now = 1000000
    mocked_time.return_value = now
    expected_rows = []
    for _ in range(5):
        expected_rows.append({
            "billing_account_id": generate_id(),
            "generated_at": random.randint(1, 10000),
            "suspend_after": 1800,
        })
    yt.write_table('//table', expected_rows)

    table_from_yt_to_s3(
        yt_proxy=yt_proxy,
        yt_token='',
        src_table_path='//table',
        s3_endpoint_url='',
        access_key_id='',
        secret_access_key='',
        bucket_name=''
    )

    with open(str(now), "r") as f:
        new_rows = []
        for row in f.readlines():
            new_rows.append(json.loads(row))

    assert expected_rows == new_rows
