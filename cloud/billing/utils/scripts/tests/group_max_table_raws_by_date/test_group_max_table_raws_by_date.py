import json
from yql.api.v1.client import YqlClient

from cloud.billing.utils.scripts.group_max_table_raws_by_date import group_max_table_raws_by_date

dates = ['1', '3', '4', '100']


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
    for i in range(1, count + 1):
        write_table(
            ytw, src_dir + '/' + str(i),
            [{'id': raw_id, 'table_id': i, 'date': date} for date in dates for raw_id in [1, 2]]
        )


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
    count = 8
    prepare_tables(ytw, src_dir, dst_dir, count=count)
    group_max_table_raws_by_date(
        yql_client=client,
        src_dir_path=src_dir,
        dst_dir_path=dst_dir
    )
    assert set(ytw.list(dst_dir)) == set(dates)
    for date in dates:
        raws = list(ytw.read_table(dst_dir + '/' + date))
        assert len(raws) == 2
        assert all([raw['table_id'] == count for raw in raws])
        assert all([raw['date'] == date for raw in raws])
        assert {raw['id'] for raw in raws} == {1, 2}
