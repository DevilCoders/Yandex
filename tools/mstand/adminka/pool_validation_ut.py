import os.path
import pytest

import yaqutils.time_helpers as utime

from adminka import pool_validation
from adminka.ab_cache import AdminkaCachedApi
from experiment_pool import Experiment
from experiment_pool import Observation
from experiment_pool import pool_helpers as phelp
from mstand_enums.mstand_online_enums import ServiceEnum


# noinspection PyClassHasNoInit
class TestPoolValidation:
    def test_wrong_id(self, session):
        obs = Observation(obs_id="3", control=None, dates=None)
        error = pool_validation.check_extra_testids(obs, session)
        assert error == "AB observation is incorrect: Has no testids"

    def test_validate_pool(self, session):
        dates = utime.DateRange.deserialize("20160105:20160115")

        exp1 = phelp.Experiment(testid="8")
        exp2 = phelp.Experiment(testid="10")

        obs1 = Observation(obs_id="9", control=exp1, experiments=[exp1], dates=dates)
        obs2 = Observation(obs_id="10", control=exp2, experiments=[exp2], dates=dates)

        pool1 = phelp.Pool([obs1])
        error1_1 = pool_validation.validate_pool(
            pool1,
            session,
            [ServiceEnum.WEB_AUTO],
        )
        assert error1_1.is_ok(), error1_1.errors

        error1_2 = pool_validation.validate_pool(
            pool1,
            session,
            ["web-desktop"],
        )
        assert error1_2.is_ok(), error1_2.errors

        pool2 = phelp.Pool([obs2])
        error2_1 = pool_validation.validate_pool(
            pool2,
            session,
            [ServiceEnum.WEB_AUTO],
        )
        expected_error2 = "TestID 10 was not enabled on these dates [20160105-20160115] for 'web-auto'."

        assert not error2_1.is_ok(), error1_1.errors
        assert error2_1.errors[obs2][0] == expected_error2

        error2_2 = pool_validation.validate_pool(
            pool2,
            session,
            [ServiceEnum.MARKET_SESSIONS_STAT],
        )
        assert error2_2.is_ok(), error2_2.errors

    def test_experiment_was_not_available(self, session):
        dates = utime.DateRange.deserialize("20160111:20160201")
        exp = phelp.Experiment(testid="8")
        obs = Observation(obs_id="9", control=exp, dates=dates)
        pool = phelp.Pool([obs])

        error = pool_validation.validate_pool(
            pool,
            session,
            [ServiceEnum.WEB_AUTO],
        )
        expected_error = "TestID 8 was not enabled on these dates [20160111-20160201] for 'all'."
        assert error.errors[obs][0] == expected_error

    def test_multiple_footprints_good(self):
        exp = Experiment(testid="1")
        footprints = [{u"restrictions": {u"platforms": u"touch",
                                         u"services": u"-avia,-extensions_vb_chrome,-intrasearch-at,-passport"}},
                      {u"restrictions": {u"platforms": u"desktop",
                                         u"services": u"-avia,-extensions_vb_chrome,-intrasearch-at,-passport"}}]

        err = pool_validation.check_restriction_for_service(footprints, "web", exp)

        assert not err

    def test_multiple_footprints_bad(self):
        exp = Experiment(testid="1")
        footprints = [{u"restrictions": {u"platforms": u"touch",
                                         u"services": u"-avia,-extensions_vb_chrome,-intrasearch-at,-passport"}},
                      {u"restrictions": {u"platforms": u"tablet",
                                         u"services": u"-avia,-extensions_vb_chrome,-intrasearch-at,-passport"}}]

        err = pool_validation.check_restriction_for_service(footprints, "web", exp)

        assert err


@pytest.fixture
def folder_path(data_path, folder):
    return os.path.join(data_path, folder)


@pytest.fixture
def pool(folder_path):
    return phelp.load_pool(os.path.join(folder_path, "pool.json"))


@pytest.fixture
def session_from_path(folder_path):
    return AdminkaCachedApi(path=os.path.join(folder_path, "cache.json"))


# noinspection PyClassHasNoInit
class TestRealPoolValidation:
    _error_pattern = r"Observation .* does not contain a list of services"

    @staticmethod
    def _check_validation_and_init(pool, session, services, use_filters=False):
        result = pool_validation.validate_pool(pool, session, services)
        assert result.is_ok(), result.pretty_print()
        result = pool_validation.init_pool_services(pool, session, services, use_filters)
        assert result.is_ok(), result.pretty_print()

    @pytest.mark.parametrize("folder", ["market_sessions_stat"])
    def test_market_sessions_stat_only(self, pool, session_from_path):
        self._check_validation_and_init(pool, session_from_path, [ServiceEnum.MARKET_SESSIONS_STAT])

    @pytest.mark.parametrize("folder", ["toloka"])
    def test_toloka_only(self, pool, session_from_path):
        self._check_validation_and_init(pool, session_from_path, [ServiceEnum.TOLOKA])

    @pytest.mark.parametrize("folder", ["web_auto"])
    def test_web_auto_only(self, pool, session_from_path):
        self._check_validation_and_init(pool, session_from_path, [ServiceEnum.WEB_AUTO])

    @pytest.mark.parametrize("folder", ["web_auto_and_morda"])
    def test_web_auto_and_morda(self, pool, session_from_path):
        self._check_validation_and_init(pool, session_from_path, [ServiceEnum.WEB_AUTO, ServiceEnum.MORDA])

    @pytest.mark.parametrize("folder", ["all_flow"])
    def test_all_flow(self, pool, session_from_path):
        self._check_validation_and_init(pool, session_from_path, list(ServiceEnum.ALL))

    @pytest.mark.parametrize("folder", ["web_auto_for_adv_testids"])
    def test_web_auto_for_adv_testids(self, pool, session_from_path):
        self._check_validation_and_init(pool, session_from_path, [ServiceEnum.WEB_AUTO], True)

    @pytest.mark.parametrize("folder", ["web_auto_for_adv_testids"])
    def test_web_auto_for_adv_testids_fail(self, pool, session_from_path):
        with pytest.raises(Exception, match=self._error_pattern):
            self._check_validation_and_init(pool, session_from_path, [ServiceEnum.WEB_TOUCH], True)
