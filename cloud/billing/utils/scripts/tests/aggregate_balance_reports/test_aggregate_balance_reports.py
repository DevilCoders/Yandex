from cloud.billing.utils.scripts.aggregate_balance_reports import aggregate_balance_reports
from cloud.dwh.test_utils.yt.misc import create_table

balance_reports_schema = {
    "balance_product_id": "string",
    "billing_account_id": "string",
    "created_at": "uint64",
    "date": "string",
    "export_ts": "uint64",
    "subaccount_id": "string",
    "total": "string"
}

var_adjustments_schema = {
    "amount": "string",
    "billing_account_id": "string",
    "created_by": "string",
    "date": "string",
    "export_ts": "uint64"
}

balance_reports_data = [{
    "balance_product_id": "509071",
    "billing_account_id": "a6q0p5slcnahkbj6o1a7",
    "created_at": 1600376661,
    "date": "2020-09-17",
    "export_ts": 1624652218,
    "subaccount_id": "",
    "total": "7.010416667"
}, {
    "balance_product_id": "509071",
    "billing_account_id": "a6q0p5slcnahkbj6o1a7",
    "created_at": 1610485536,
    "date": "2021-01-12",
    "export_ts": 1624652218,
    "subaccount_id": "",
    "total": "75.333684099"
}, {
    "balance_product_id": "509071",
    "billing_account_id": "a6q0p5slcnahkbj6o1a7",
    "created_at": 1611781968,
    "date": "2021-01-27",
    "export_ts": 1624652218,
    "subaccount_id": "",
    "total": "75.330055932"
}, {
    "balance_product_id": "509071",
    "billing_account_id": "a6q0p5slcnahkbj6oooo",
    "created_at": 1613855545,
    "date": "2021-02-20",
    "export_ts": 1624652218,
    "subaccount_id": "",
    "total": "78.44038316"
}]

var_adjustments_data = [{
    "amount": "33.33",
    "billing_account_id": "a6q0p5slcnahkbj6o1a7",
    "created_by": "bfbvi6hanq5bmk189v3u",
    "date": "2021-01-27",
    "export_ts": 1624648435
}]

result_jan_12_data = [{
    "date": "2021-01-12",
    "amount": "75.333684099",
    "product_id": "509071",
    "project_id": "a6q0p5slcnahkbj6o1a7"
}]

result_jan_27_data = [{
    "date": "2021-01-27",
    "amount": "108.660055932",
    "product_id": "509071",
    "project_id": "a6q0p5slcnahkbj6o1a7"
}]

result_feb_20_data = [{
    "date": "2021-02-20",
    "amount": "78.44038316",
    "product_id": "509071",
    "project_id": "a6q0p5slcnahkbj6oooo"
}]


def test_aggregate_balance_reports_jan_with_adj(yql_client, yt_client, test_prefix):
    date_from = "2021-01-01"
    date_to = "2021-01-31"
    env = "test"
    dst_dir = test_prefix + "/balance_cloud_completions_test"

    create_table(yt_client,
                 path="exported-billing-tables/balance_reports_test",
                 schema=balance_reports_schema,
                 data=balance_reports_data,
                 test_prefix=test_prefix)

    create_table(yt_client,
                 path="exported-billing-tables/var_adjustments_test",
                 schema=var_adjustments_schema,
                 data=var_adjustments_data,
                 test_prefix=test_prefix)

    aggregate_balance_reports(
        table_path_prefix=test_prefix,
        date_from=date_from,
        date_to=date_to,
        env=env,
        yql_client=yql_client)

    dir_content = list(yt_client.list(dst_dir))
    assert len(dir_content) == 31
    assert list(yt_client.read_table(dst_dir + '/2021-01-01')) == []
    assert list(yt_client.read_table(dst_dir + '/2021-01-10')) == []
    assert list(yt_client.read_table(dst_dir + '/2021-01-12')) == result_jan_12_data
    assert list(yt_client.read_table(dst_dir + '/2021-01-15')) == []
    assert list(yt_client.read_table(dst_dir + '/2021-01-27')) == result_jan_27_data
    assert list(yt_client.read_table(dst_dir + '/2021-01-31')) == []


def test_aggregate_balance_reports_feb_without_adj(yql_client, yt_client, test_prefix):
    date_from = "2021-02-01"
    date_to = "2021-02-28"
    env = "test"
    dst_dir = test_prefix + "/balance_cloud_completions_test"

    create_table(yt_client,
                 path="exported-billing-tables/balance_reports_test",
                 schema=balance_reports_schema,
                 data=balance_reports_data,
                 test_prefix=test_prefix)

    create_table(yt_client,
                 path="exported-billing-tables/var_adjustments_test",
                 schema=var_adjustments_schema,
                 data=var_adjustments_data,
                 test_prefix=test_prefix)

    aggregate_balance_reports(
        table_path_prefix=test_prefix,
        date_from=date_from,
        date_to=date_to,
        env=env,
        yql_client=yql_client)

    dir_content = list(yt_client.list(dst_dir))
    assert len(dir_content) == 28
    assert list(yt_client.read_table(dst_dir + '/2021-02-19')) == []
    assert list(yt_client.read_table(dst_dir + '/2021-02-20')) == result_feb_20_data
    assert list(yt_client.read_table(dst_dir + '/2021-02-21')) == []
