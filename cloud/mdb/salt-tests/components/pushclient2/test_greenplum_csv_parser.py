import yatest
import pytest
from cloud.mdb.salt.salt.components.pushclient2.parsers.greenplum_csv_parser import readlines, detect_magic

TEST_DATA_FILE_PATH = 'cloud/mdb/salt-tests/components/pushclient2/test_data/test_greenplum_csv_parser_data.txt'


def test_readlines():
    with open(yatest.common.source_path(TEST_DATA_FILE_PATH), 'r') as source:
        assert (
            next(readlines(source=source, misc_data={}))
            == '2022-03-17 00:33:40.030099 MSK, "string with unsupported utf-8 symbols;" '
        )
        assert (
            next(readlines(source=source, misc_data={}))
            == '2022-03-17 00:33:40.030099 MSK, "string without unsupported utf-8 symbols;" '
        )


def test_detect_magic():
    line_with_magic = '123;123;123;select * from super_schema.super_table;'
    line_without_magic = 'select * from super_schema.super_table;'
    detect_magic(line_with_magic) == [[123, 123, 123], 'select * from super_schema.super_table;']
    with pytest.raises(ValueError):
        detect_magic(line_without_magic)
