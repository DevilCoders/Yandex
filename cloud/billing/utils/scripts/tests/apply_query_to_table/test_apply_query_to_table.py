from cloud.billing.utils.scripts.apply_query_to_table import apply_query_to_table
from yql.api.v1.client import YqlClient

RAW = {
    "key": "key",
    "value": "value"
}
RAW2 = {
    "key": "another_key",
    "value": "another_value"
}


def write_table(yql_client, dir, table, raws):
    keys = list(raws[0].keys())
    values = [["'" + str(raw[k]) + "'" for k in keys] for raw in raws]
    query = '$dst = "{}"; INSERT INTO $dst ({}) VALUES {}'
    query = query.format(
        dir + "/" + table,
        ", ".join(keys),
        ", ".join(["(" + ", ".join(raw) + ")" for raw in values])
    )
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


def test_select_query(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_1'
    dst_dir = 'dst_dir_1'
    dst_table_1 = dst_dir + '/' + "result1"
    dst_table_2 = dst_dir + '/' + "result2"
    create_dirs(ytw, src_dir, dst_dir)
    write_table(client, src_dir, "1", [RAW])
    apply_query_to_table(yql_client=client, yt_wrapper=ytw,
                         query="$src_table = '{}'; $dst_table = '{}'; $result = (SELECT * FROM $src_table); ".format(src_dir + "/1", dst_table_2),
                         outputs="result:{},result:dst_table".format(dst_table_1))
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 2
    assert list(ytw.read_table(dst_table_1)) == [RAW, ]
    assert list(ytw.read_table(dst_table_2)) == [RAW, ]


def test_join_query(tmpdir, yql_api, mongo, yt):
    ytw = yt.yt_wrapper
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy=ytw.config["proxy"]["url"]
    )
    src_dir = 'src_dir_2'
    dst_dir = 'dst_dir_2'
    dst_table = dst_dir + '/result'
    create_dirs(ytw, src_dir, dst_dir)

    write_table(client, src_dir, "1", [RAW])
    raw1 = RAW.copy()
    raw1["value"] = RAW2["value"]
    write_table(client, src_dir, "2", [raw1, RAW2])
    query = "$src_table = '{}'; $result = (SELECT a.*, b.value as b_value FROM $src_table as a JOIN `src_dir_2/2` as b USING (key));"
    apply_query_to_table(yql_client=client, yt_wrapper=ytw, query=query.format(src_dir + "/1"),
                         outputs="result:" + dst_table)
    dir_content = list(ytw.list(dst_dir))
    assert len(dir_content) == 1
    result = list(ytw.read_table(dst_table))
    assert len(result) == 1
    assert result[0] == {
        "key": "key",
        "value": "value",
        "b_value": "another_value"
    }
