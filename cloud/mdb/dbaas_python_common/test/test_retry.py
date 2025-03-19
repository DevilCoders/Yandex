"""
Test for our retry util
"""

import pytest

from dbaas_common import retry


class FooError(Exception):
    pass


class Test_retry_on_exception:
    small_wait = 0.0000000001

    @pytest.mark.parametrize(
        'exception',
        [
            FooError,
            (FooError, IOError),
        ],
    )
    def test_reraise_exception(self, exception):
        @retry.on_exception(exception, factor=self.small_wait)
        def tgt():
            raise FooError

        with pytest.raises(FooError):
            tgt()

    def test_max_tries(self):
        calls = [0]

        @retry.on_exception(FooError, factor=self.small_wait, max_tries=5)
        def tgt():
            calls[0] = calls[0] + 1
            raise FooError

        with pytest.raises(FooError):
            tgt()

        assert calls[0] == 5, 'Should be five calls'

    def test_giveup(self):
        calls = [0]

        @retry.on_exception(FooError, factor=self.small_wait, giveup=lambda e: e.args[0] == 'it')
        def tgt():
            calls[0] = calls[0] + 1
            raise FooError('it')

        with pytest.raises(FooError):
            tgt()

        assert calls[0] == 1, 'Should be only one calls'
