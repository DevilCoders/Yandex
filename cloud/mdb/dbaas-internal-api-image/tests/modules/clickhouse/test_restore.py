"""
ClickHouse restore utils fixes
"""

from dbaas_internal_api.modules.clickhouse.restore import options_which_affect_ddl


class Test_options_which_affect_ddl:  # pylint: disable=invalid-name
    """
    test for options_which_affect_ddl
    """

    # pylint: disable=missing-docstring

    def test_returns_affected_options(self):
        graphite_rollup_option = {
            'graphite_rollup': [
                {
                    'name': 'test',
                    'patterns': [
                        {
                            'function': 'any',
                            'retention': [
                                {
                                    'age': 60,
                                    'precision': 1,
                                }
                            ],
                        }
                    ],
                }
            ],
        }

        assert options_which_affect_ddl(dict(log_level='debug', **graphite_rollup_option)) == graphite_rollup_option

    def test_return_nothing_when_no_affected_options(self):
        assert (
            options_which_affect_ddl(
                {
                    'log_level': 'debug',
                    'mark_cache_size': 42,
                }
            )
            == {}
        )
