import pytest

import yaqutils.time_helpers as utime

from adminka import pool_enrich_services
from experiment_pool import pool_helpers as phelp
from mstand_enums.mstand_online_enums import ServiceEnum


# noinspection PyClassHasNoInit
class TestExtendServicesFromObservation:
    _error_pattern = r"^Observation .* is not available for services .*\.$"

    @staticmethod
    def _init_observation(testid, obs_id, dates=None):
        if dates is None:
            dates = utime.DateRange.deserialize("20160105:20160115")
        elif isinstance(dates, str):
            dates = utime.DateRange.deserialize(dates)
        control = phelp.Experiment(testid=testid)
        return phelp.Observation(obs_id=obs_id, control=control, dates=dates)

    def test_extend_services_web_auto(self, session):
        obs = self._init_observation(testid="4", obs_id="9")
        services = [ServiceEnum.WEB_AUTO]

        # noinspection PyTypeChecker
        services_received = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
        )
        assert set(services_received) == {"touch", "web"}

    def test_extend_services_web_auto_extended(self, session):
        obs = self._init_observation(testid="4", obs_id="9")
        services = [ServiceEnum.WEB_AUTO_EXTENDED]

        # noinspection PyTypeChecker
        services_new = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
        )
        assert set(services_new) == {"web-touch-extended", "web-desktop-extended"}

    def test_extend_services_observation_uitype_filters_desktop(self, session):
        obs = self._init_observation(testid="4", obs_id="7")
        services = [ServiceEnum.WEB_AUTO]

        # noinspection PyTypeChecker
        services_new = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
            use_filters=True,
        )
        assert set(services_new) == {"web"}

    def test_extend_services_observation_uitype_filters_touch(self, session):
        obs = self._init_observation(testid="4", obs_id="8")
        services = [ServiceEnum.WEB_AUTO]

        # noinspection PyTypeChecker
        services_new = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
            use_filters=True,
        )
        assert set(services_new) == {"touch"}

    def test_extend_services_empty_platform(self, session):
        obs = self._init_observation(testid="8", obs_id="9")
        services = [ServiceEnum.WEB_AUTO, ServiceEnum.WEB_AUTO_EXTENDED]

        # noinspection PyTypeChecker
        services_new = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
        )
        assert set(services_new) == {"web", "web-desktop-extended", "touch", "web-touch-extended"}

    def test_extend_services_without_auto(self, session):
        obs = self._init_observation(testid="8", obs_id="9")
        services = [ServiceEnum.WEB_DESKTOP, ServiceEnum.WEB_TOUCH]

        # noinspection PyTypeChecker
        services_new = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
        )
        assert set(services_new) == {"web", "touch"}

    def test_extend_services_aliases(self, session):
        obs = self._init_observation(testid="8", obs_id="9")
        services = ["web-desktop", "web-touch"]

        # noinspection PyTypeChecker
        services_new = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
        )
        assert set(services_new) == {"web", "touch"}

    def test_extend_services_dates_too_much(self, session):
        obs = self._init_observation(testid="4", obs_id="9", dates="20160105:20160205")
        services = [ServiceEnum.WEB_AUTO, ServiceEnum.ALICE]

        with pytest.raises(Exception, match=self._error_pattern):
            # noinspection PyTypeChecker
            pool_enrich_services.extend_services_from_observation(
                obs,
                session,
                services,
            )

    def test_extend_services_one_event_from_two_is_ok(self, session):
        obs = self._init_observation(testid="11", obs_id="11", dates="20160105:20160108")
        services = [ServiceEnum.MARKET_SESSIONS_STAT]

        # noinspection PyTypeChecker
        services_new = pool_enrich_services.extend_services_from_observation(
            obs,
            session,
            services,
        )
        assert set(services_new) == {ServiceEnum.MARKET_SESSIONS_STAT}

    def test_extend_services_until_less_off(self, session):
        obs1 = self._init_observation(testid="11", obs_id="11", dates="20160110:20160113")
        obs2 = self._init_observation(testid="11", obs_id="11", dates="20160110:20160115")
        services = [ServiceEnum.MARKET_WEB_REQID]

        # noinspection PyTypeChecker
        services_new_1 = pool_enrich_services.extend_services_from_observation(
            obs1,
            session,
            services,
        )
        assert set(services_new_1) == {ServiceEnum.MARKET_WEB_REQID}

        # noinspection PyTypeChecker
        services_new_2 = pool_enrich_services.extend_services_from_observation(
            obs2,
            session,
            services,
        )
        assert set(services_new_2) == {ServiceEnum.MARKET_WEB_REQID}

    def test_extend_services_web_fail(self, session):
        obs = self._init_observation(testid="11", obs_id="11", dates="20160110:20160113")
        services = [ServiceEnum.WEB_DESKTOP]

        with pytest.raises(Exception, match=self._error_pattern):
            # noinspection PyTypeChecker
            pool_enrich_services.extend_services_from_observation(
                obs,
                session,
                services,
            )

    def test_extend_services_images_fail(self, session):
        obs = self._init_observation(testid="11", obs_id="11", dates="20160110:20160113")
        services = [ServiceEnum.IMAGES]

        with pytest.raises(Exception, match=self._error_pattern):
            # noinspection PyTypeChecker
            pool_enrich_services.extend_services_from_observation(
                obs,
                session,
                services,
            )
