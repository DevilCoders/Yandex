import pytest
import yatest.common
from hamcrest import assert_that, has_items, equal_to, starts_with
from cloud.mdb.salt.salt.components.dom0porto.conf.heartbeat import guess_generation, get_cpu_model


def test_get_cpu_model_from_real_example():
    cpuinfo = yatest.common.source_path('cloud/mdb/salt/salt/components/dom0porto/conf/tests/cpuinfo.txt')
    assert get_cpu_model(cpuinfo) == ('Intel(R) Xeon(R) Gold 6338 CPU @ 2.00GHz', [])


def test_get_cpu_model_from_non_existed_file():
    assert_that(
        get_cpu_model('Non_Existed_File'),
        has_items(equal_to(''), has_items(starts_with("Unable to get cpu model from 'Non_Existed_File'"))),
    )


def test_get_cpu_model_from_empty_file():
    ya_make = yatest.common.source_path('cloud/mdb/salt/salt/components/dom0porto/conf/tests/ya.make')

    assert_that(get_cpu_model(ya_make), has_items(equal_to(''), has_items(starts_with("cpu model not found in '"))))


@pytest.mark.parametrize(
    ['net_speed', 'num_cpu', 'cpu_model', 'generation'],
    [
        [6553600000, 128, 'Intel(R) Xeon(R) Gold 6338 CPU @ 2.00GHz', 4],
        [3276800000, 80, 'Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz', 3],
        [1310720000, 56, 'Intel(R) Xeon(R) CPU E5-2660 v4 @ 2.00GHz', 2],
        [131072000, 32, 'Intel(R) Xeon(R) CPU E5-2650 v2 @ 2.60GHz', 1],
        [131072000, 24, 'AMD Ryzen 5 1600X Six-Core Processor', 1],
    ],
)
def test_guess_generation(net_speed, num_cpu, cpu_model, generation):
    assert guess_generation(net_speed, num_cpu, cpu_model) == generation
