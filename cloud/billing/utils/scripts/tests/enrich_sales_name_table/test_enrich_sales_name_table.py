import json
import os

import mock
from cloud.billing.utils.scripts.enrich_sales_name_table import enrich_sales_name_table


@mock.patch("requests.get", autospec=True)
def test_full_case(mocked_get):
    import yt.wrapper as yt
    yt_proxy = os.environ["YT_PROXY"]
    yt.config['proxy']['url'] = yt_proxy

    expected_data = [
        {
            "login": "login1",
            "billing_account_id": "baid1",
            "first_name_ru": "firstnameru1",
            "first_name_en": "firstnameen1",
            "last_name_ru": "lastnameru1",
            "last_name_en": "lastnameen1",
            "work_email": "workemail1",
            "position_ru": "positionru1",
            "position_en": "positionen1"
        },
        {
            "login": "login1",
            "billing_account_id": "baid2",
            "first_name_ru": "firstnameru1",
            "first_name_en": "firstnameen1",
            "last_name_ru": "lastnameru1",
            "last_name_en": "lastnameen1",
            "work_email": "workemail1",
            "position_ru": "positionru1",
            "position_en": "positionen1"
        },
        {
            "login": "login3",
            "billing_account_id": "baid3",
            "first_name_ru": "firstnameru3",
            "first_name_en": "firstnameen3",
            "last_name_ru": "lastnameru3",
            "last_name_en": "lastnameen3",
            "work_email": "workemail3",
            "position_ru": "positionru3",
            "position_en": "positionen3"
        },
        {
            "login": "",
            "billing_account_id": "baid1",
            "first_name_ru": "",
            "first_name_en": "",
            "last_name_ru": "",
            "last_name_en": "",
            "work_email": "",
            "position_ru": "",
            "position_en": ""
        }
    ]
    mocked_get.return_value.text = json.dumps({
        "result": [
            {
                "official": {
                    "position": {
                        "ru": row["position_ru"],
                        "en": row["position_en"]
                    }
                },
                "login": row["login"],
                "work_email": row["work_email"],
                "name": {
                    "first": {
                        "ru": row["first_name_ru"],
                        "en": row["first_name_en"]
                    },
                    "last": {
                        "ru": row["last_name_ru"],
                        "en": row["last_name_en"]
                    }
                }
            }
            for row in expected_data[1:3]
        ]
    })
    yt_data = [{"billing_account_id": row["billing_account_id"],
                "sales_name": row["login"]} for row in expected_data]
    src_table_path = "//src_table"
    dst_table_path = "//dst_table"
    src_table = yt.TablePath(src_table_path)
    dst_table = yt.TablePath(dst_table_path)
    yt.write_table(
        src_table,
        '\n'.join(map(json.dumps, yt_data)).encode("utf-8"),
        raw=True,
        format="json"
    )
    enrich_sales_name_table(
        src_table_path=src_table_path,
        dst_table_path=dst_table_path,
        yt_token="TOKEN",
        yt_proxy=yt_proxy,
        staff_token="TOKEN",
        staff_url="URL"
    )
    assert list(yt.read_table(dst_table, format=yt.JsonFormat())) == expected_data
    mocked_get.assert_called_with("URL", headers={'Authorization': 'OAuth ' + "TOKEN"}, params={
        "login": "login3,login1",
        "_fields": "login,work_email,official.position,name.first,name.last"
    })
