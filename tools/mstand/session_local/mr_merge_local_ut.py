import session_local.mr_merge_local


# noinspection PyClassHasNoInit
class TestMRMergeLocal:
    def test_simple(self):
        table1 = [
            {
                "key": 1,
                "value": 1,
            },
            {
                "key": 1,
                "value": 2,
            },
            {
                "key": 20,
                "value": 3,
            },
            {
                "key": 21,
                "value": 4,
            },
        ]
        table2 = [
            {
                "key": 5,
                "value": -1,
            },
            {
                "key": 20,
                "value": -2,
            },
            {
                "key": 21,
                "value": -3,
            },
            {
                "key": 30,
                "value": -4,
            },
        ]
        expected = [
            {
                "key": 1,
                "value": 1,
                "@table_index": 0,
            },
            {
                "key": 1,
                "value": 2,
                "@table_index": 0,
            },
            {
                "key": 5,
                "value": -1,
                "@table_index": 1,
            },
            {
                "key": 20,
                "value": 3,
                "@table_index": 0,
            },
            {
                "key": 20,
                "value": -2,
                "@table_index": 1,
            },
            {
                "key": 21,
                "value": 4,
                "@table_index": 0,
            },
            {
                "key": 21,
                "value": -3,
                "@table_index": 1,
            },
            {
                "key": 30,
                "value": -4,
                "@table_index": 1,
            },
        ]
        merged = list(session_local.mr_merge_local.merge_iterators([table1, table2], keys=["key"]))
        assert merged == expected
