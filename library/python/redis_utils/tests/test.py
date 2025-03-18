from library.python.redis_utils import RedisSentinelSettings


class TestRedisSentinelSettings:
    def make_settings(self):
        return RedisSentinelSettings(
            hosts=['host1', 'host2'],
            cluster_name='name',
            password='pass',
        )

    def test_get_raw_config(self):
        assert self.make_settings().make_connection_url() == ('sentinel://:pass@host1:26379/0;'
                                                              'sentinel://:pass@host2:26379/0;')

    def test_make_celery_broker_transport_options(self):
        assert self.make_settings().make_celery_broker_transport_options() == {'master_name': 'name'}
