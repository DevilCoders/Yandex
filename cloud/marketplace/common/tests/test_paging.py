import pytest

from yc_common.clients.kikimr.sql import SqlIn
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args

_in = SqlIn("bar", [1, 2, 3])


@pytest.mark.parametrize("args, kwargs, wq, wa", [
    ((None, None), {"id": "id", "filter_query": "foo = ?", "filter_args": [1]}, " WHERE foo = ? ORDER BY id", [1]),
    (("c1", None), {"id": "id", "filter_query": "foo = ? AND ?", "filter_args": [1, _in], "order_by": "name"},
     " WHERE (foo = ? AND ?) AND id > ? ORDER BY name, id", [1, _in, "c1"]),
    (("c1", None), {"id": "id"}, " WHERE id > ? ORDER BY id", ["c1"]),
])
def test_parse_filter_wrong_argument_type(args, kwargs, wq, wa):
    where_query, where_args = page_query_args(*args, **kwargs)
    assert where_query == wq
    assert where_args == wa
