import glob
import os.path
import pytest

import yql.library.fastcheck.python as fastcheck
import yatest.common

YQL_PATH = 'cloud/analytics/yql'
YQL_EXTENSION = 'yql'
CLUSTER_MAPPING = {
    'hahn': 'yt',
    'vanga': 'yt',
    'stat': 'statface',
}
TESTING_YT_ACCOUNT = '//home/cloud_analytics_test'
YQL_SYNTAX_VERSIONS = [0, 1]
DEFAULT_YT_CLUSTER = 'hahn'
DEFAULT_CLEANUP_INTERVAL = 30

yql_filenames = glob.glob(yatest.common.source_path(os.path.join(YQL_PATH, '*.'+YQL_EXTENSION)))


def form_query(query):
    yql_query_placeholders = {
        "%YT_CLUSTER%": DEFAULT_YT_CLUSTER,
        "%CLEANUP_INTERVAL%": DEFAULT_CLEANUP_INTERVAL,
    }

    for key, value in yql_query_placeholders.iteritems():
        query = query.replace(key, str(value))

    return query


@pytest.mark.parametrize('yql_filename', yql_filenames, ids=os.path.basename)
def test_basic(yql_filename):
    with open(yql_filename) as yql_file:
        cmd = form_query(yql_file.read())

        errors = []
        assertion = False
        for syntax_version in YQL_SYNTAX_VERSIONS:
            assertion = assertion or fastcheck.check_program(
                cmd,
                cluster_mapping=CLUSTER_MAPPING,
                errors=errors,
                syntax_version=syntax_version,
            )
            if assertion:
                break

        assert assertion, "\n".join((str(e) for e in errors))

        # TODO(syndicut): Enable it sometime
        # assert TESTING_YT_ACCOUNT not in cmd, 'Thou shall not use testing data in production'
