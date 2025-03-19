import json

from yql.api.v1.client import YqlClient

from cloud.billing.utils.scripts.aggregate_billing_export_table import aggregate_billing_export_table

RAW1 = {
    'pk1': 1,
    'pk2': 1,
    'pk3': 1,
    'other1': 1,
    'other2': 1,
    'other3': 1,
    'time_field': 9
}

RAW2 = {
    'pk1': 2,
    'pk2': 2,
    'pk3': 2,
    'other1': 2,
    'other2': 2,
    'other3': 2,
    'time_field': 9
}

PKS = ["pk1", "pk2", "pk3"]
OTHER = ["other1", "other2", "other3"]
TIME_FIELD = "time_field"


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
        new_raw1[TIME_FIELD] = new_raw2[TIME_FIELD] = i
        write_table(ytw, src_dir + '/' + str(i), [new_raw1, new_raw2])


def test_positive(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir_1 = 'src_dir_1'
    dst_dir = 'dst_dir_1'
    src_dir_2 = 'src_dir_2'
    prepare_tables(ytw, src_dir_1, dst_dir)
    ytw.mkdir(src_dir_2, recursive=True)
    new_raw_1 = RAW1.copy()
    new_raw_1.update({TIME_FIELD: 100})
    new_raw_2 = RAW2.copy()
    new_raw_2.update({TIME_FIELD: 100})
    write_table(ytw, src_dir_2 + '/0', [new_raw_1, new_raw_2])
    aggregate_billing_export_table(src_dir_path=src_dir_1, dst_table_path=dst_dir + "/result",
                                   primary_key_fields=PKS,
                                   other_fields=OTHER,
                                   export_time_field=TIME_FIELD,
                                   yql_client=client, yt_wrapper=ytw)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    assert list(ytw.read_table(dst_dir + '/result')) == [RAW1, RAW2]

    aggregate_billing_export_table(src_dir_path=src_dir_2, dst_table_path=dst_dir + "/result",
                                   primary_key_fields=PKS,
                                   other_fields=OTHER,
                                   export_time_field=TIME_FIELD,
                                   yql_client=client, yt_wrapper=ytw)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    assert list(ytw.read_table(dst_dir + '/result')) == [new_raw_1, new_raw_2]


def test_different_export_ts(tmpdir, yql_api, mongo, yt):
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
    expected_raw_2 = RAW2.copy()
    expected_raw_2[TIME_FIELD] = 7
    aggregate_billing_export_table(src_dir_path=src_dir, dst_table_path=dst_dir + "/result",
                                   primary_key_fields=PKS,
                                   other_fields=OTHER,
                                   export_time_field=TIME_FIELD,
                                   yql_client=client, yt_wrapper=ytw)
    assert len(list(ytw.list(dst_dir))) == 1
    assert list(ytw.read_table(dst_dir + '/result')) == [RAW1, expected_raw_2]
