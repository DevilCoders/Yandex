import random
import os
import mock

from cloud.billing.utils.scripts.upload_test_data_to_yt import upload_test_data_to_yt


def generate_id():
    alph = "0123456789qwertyuiopasdfghjklzxcvbnm"
    s = ''
    for i in range(16):
        s += random.choice(alph)
    return s


@mock.patch("time.time")
def test_full_case(mocked_time):
    import yt.wrapper as yt
    yt_proxy = os.environ["YT_PROXY"]
    now = 1000000
    mocked_time.return_value = now
    expected_rows = []
    for _ in range(5):
        expected_rows.append({
            "billing_account_id": generate_id(),
            "generated_at": now,
            "suspend_after": 1800,
        })

    upload_test_data_to_yt(yt_proxy, "", "//dst", " ".join(item["billing_account_id"] for item in expected_rows))
    rows = yt.read_table("//dst")
    assert list(rows) == expected_rows
