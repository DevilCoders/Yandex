from cloud.billing.utils.scripts.aggregate_billing_records import aggregate_billing_records
from yql.api.v1.client import YqlClient

RAW1 = {
    'billing_account_id': 1,
    'end_time': 1,
    'cloud_id': 1,
    'sku_id': 1,
    'labels_hash': 1,
    'start_time': 1,
    'pricing_quantity': 1,
    'cost': 1,
    'credit': 1,
    'credit_charges': 1,
    'created_at': 1
}

RAW2 = {
    'billing_account_id': 2,
    'end_time': 2,
    'cloud_id': 2,
    'sku_id': 2,
    'labels_hash': 2,
    'start_time': 2,
    'pricing_quantity': 2,
    'cost': 2,
    'credit': 2,
    'credit_charges': 2,
    'created_at': 2
}
PK = ["billing_account_id",
      "end_time",
      "cloud_id",
      "sku_id",
      "labels_hash"]


def write_table(yql_client, dir, table, raws):
    keys = list(raws[0].keys())
    values = [[str(raw[k]) for k in keys] for raw in raws]
    query = '$dst = "{}"; INSERT INTO $dst ({}) VALUES {}'
    query = query.format(
        dir + "/" + table,
        ", ".join(keys),
        ", ".join(["(" + ", ".join(raw) + ")" for raw in values])
    )
    print(query)
    request = yql_client.query(query, syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))


def create_dirs(ytw, *args):
    for dir in args:
        ytw.mkdir(dir)


def test_dst_table_not_exists(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_1'
    dst_dir = 'dst_dir_1'
    create_dirs(ytw, src_dir, dst_dir)
    write_table(client, src_dir, "1", [RAW1])
    write_table(client, src_dir, "2", [RAW2])
    aggregate_billing_records(src_dir_path=src_dir, dst_table_path=dst_dir + "/billing_records",
                              yql_client=client, yt_wrapper=ytw)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    assert list(ytw.read_table(dst_dir + '/' + "billing_records")) in [[RAW1, RAW2], [RAW2, RAW1]]


def test_group_by(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_2'
    dst_dir = 'dst_dir_2'
    create_dirs(ytw, src_dir, dst_dir)
    raw2 = RAW1.copy()
    raw2.update({
        'start_time': 2,
        'pricing_quantity': 2,
        'cost': 2,
        'credit': 2,
        'credit_charges': 2,
        'created_at': 2
    })
    write_table(client, src_dir, "1", [RAW1])
    write_table(client, src_dir, "2", [raw2])
    aggregate_billing_records(src_dir_path=src_dir, dst_table_path=dst_dir + "/billing_records",
                              yql_client=client, yt_wrapper=ytw)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    result = list(ytw.read_table(dst_dir + '/' + "billing_records"))
    assert len(result) == 1
    for k, v in result[0].items():
        if k in PK:
            assert v == RAW1[k]
        else:
            assert v in [1, 2]


def test_table_exists_null_intersection(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_3'
    dst_dir = 'dst_dir_3'
    create_dirs(ytw, src_dir, dst_dir)
    write_table(client, src_dir, "1", [RAW1])
    write_table(client, dst_dir, "billing_records", [RAW2])
    aggregate_billing_records(src_dir_path=src_dir, dst_table_path=dst_dir + "/billing_records",
                              yql_client=client, yt_wrapper=ytw)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    assert list(ytw.read_table(dst_dir + '/' + "billing_records")) in [[RAW2, RAW1], [RAW1, RAW2]]


def test_table_exists_not_null_intersection(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_4'
    dst_dir = 'dst_dir_4'
    create_dirs(ytw, src_dir, dst_dir)
    write_table(client, src_dir, "1", [RAW1, RAW2])
    raw1 = RAW1.copy()
    raw1.update({
        'start_time': 2,
        'pricing_quantity': 2,
        'cost': 2,
        'credit': 2,
        'credit_charges': 2,
        'created_at': 2
    })
    write_table(client, dst_dir, "billing_records", [raw1])
    aggregate_billing_records(src_dir_path=src_dir, dst_table_path=dst_dir + "/billing_records",
                              yql_client=client, yt_wrapper=ytw)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    assert list(ytw.read_table(dst_dir + '/' + "billing_records")) in [[raw1, RAW2], [RAW2, raw1]]
