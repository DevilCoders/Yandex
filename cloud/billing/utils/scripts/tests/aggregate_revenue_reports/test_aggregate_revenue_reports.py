import json

from yql.api.v1.client import YqlClient

from cloud.billing.utils.scripts.aggregate_revenue_reports import aggregate_revenue_reports

RAW1 = {
    'publisher_account_id': 1,
    'date': 1,
    'sku_id': 1,
    'billing_account_id': 1,
    'publisher_balance_client_id': 1,
    'total': 1,
    'created_at': 1,
    'export_ts': 9
}

RAW2 = {
    'publisher_account_id': 2,
    'date': 2,
    'sku_id': 2,
    'billing_account_id': 2,
    'publisher_balance_client_id': 2,
    'total': 2,
    'created_at': 2,
    'export_ts': 9
}


def write_table(ytw, path, raws):
    ytw.write_table(
        path,
        '\n'.join(map(json.dumps, raws)).encode('utf-8'),
        raw=True,
        format="json"
    )


def prepare_tables(ytw, src_dir, dst_dir, count=10):
    ytw.mkdir(src_dir, recursive=True)
    ytw.mkdir(dst_dir, recursive=True)
    for i in range(count):
        new_raw1 = RAW1.copy()
        new_raw2 = RAW2.copy()
        new_raw1['export_ts'] = new_raw2['export_ts'] = i
        write_table(ytw, src_dir + '/' + str(i), [new_raw1, new_raw2])


def test_positive(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_1'
    dst_dir = 'dst_dir_1'
    prepare_tables(ytw, src_dir, dst_dir)
    aggregate_revenue_reports(dst_dir_path=dst_dir, src_dir_path=src_dir, yql_client=client, yt_wrapper=ytw)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    assert list(ytw.read_table(dst_dir + '/' + dir_content[0])) == [RAW1, RAW2]


def test_wrong_export_ts(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_3'
    dst_dir = 'dst_dir_3'
    prepare_tables(ytw, src_dir, dst_dir, count=8)
    write_table(ytw, src_dir + '/9', [RAW1])
    aggregate_revenue_reports(dst_dir_path=dst_dir, src_dir_path=src_dir, yql_client=client, yt_wrapper=ytw)
    assert list(ytw.list(dst_dir)) == []
