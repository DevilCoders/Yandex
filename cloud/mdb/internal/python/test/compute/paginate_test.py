import pytest

from cloud.mdb.internal.python.compute.pagination import paginate, ComputeResponse, ComputeRequest


SOURCE = ['item-1', 'item-2']


def test_pagination_contract(mocker):
    def read_from_source(req: ComputeRequest) -> ComputeResponse:
        if req.page_token == '':
            index = 0
        else:
            index = int(req.page_token)
        if index >= len(SOURCE) - 1:
            next_token = ''
        else:
            next_token = str(index + 1)
        if index >= len(SOURCE):
            resources = []
        else:
            resources = [SOURCE[index]]
        return ComputeResponse(
            next_page_token=next_token,
            resources=resources,
        )

    reader = mocker.Mock()
    reader.side_effect = read_from_source

    class TestRequest:
        page_token = ''

    req = TestRequest()
    result = paginate(reader, req)
    assert next(result) == 'item-1'
    assert next(result) == 'item-2'
    with pytest.raises(StopIteration):
        next(result)
    assert reader.call_count == 2
