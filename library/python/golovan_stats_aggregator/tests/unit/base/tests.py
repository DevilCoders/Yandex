# -*- coding: utf-8 -*-
import unittest
from unittest import TestCase

from hamcrest import (
    has_items,
    assert_that,
    equal_to,
    empty,
    has_item,
    calling,
    contains,
    raises
)


class _BaseStatsAggregator(TestCase):
    implementation_class = None

    def setUp(self):
        if not self.implementation_class:
            raise unittest.SkipTest('skipping base class')

        super(_BaseStatsAggregator, self).setUp()
        self.golovan_aggregator = self.get_stats_aggregator()

    def get_stats_aggregator(self, left_border=0.001, right_border=1000, progression_step=1.5):
        return self.implementation_class(
            left_border=left_border,
            right_border=right_border,
            progression_step=progression_step
        )

    def test_set_should_rewrite_existing_metric_value_in_cache(self):
        key = 'errors_count_summ'
        value = 1
        new_value = 2

        self.golovan_aggregator.set(key, value)

        assert_that(
            self.golovan_aggregator._cache_get(key),
            equal_to(value)
        )
        self.golovan_aggregator.set(key, new_value)
        assert_that(
            self.golovan_aggregator._cache_get(key),
            equal_to(new_value)
        )

    def test_inc_should_increment_metric_in_cache(self):
        key = 'errors_count_summ'
        base_value = 10
        inc_amount = 2

        assert_that(
            self.golovan_aggregator.get_num_metric_names(),
            empty()
        )
        self.golovan_aggregator.set(key, base_value)

        assert_that(
            self.golovan_aggregator.get_num_metric_names(),
            has_item(key)
        )
        assert_that(
            self.golovan_aggregator._cache_get(key),
            equal_to(base_value)
        )
        self.golovan_aggregator.inc(key, inc_amount)
        assert_that(
            self.golovan_aggregator._cache_get(key),
            equal_to(base_value + inc_amount)
        )

    def test_inc_should_increment_metric_in_cache_by_one(self):
        key = 'errors_count_summ'

        self.golovan_aggregator.inc(metric_name=key)
        assert_that(
            self.golovan_aggregator._cache_get(key),
            equal_to(1)
        )
        self.golovan_aggregator.inc(metric_name=key)
        assert_that(
            self.golovan_aggregator._cache_get(key),
            equal_to(2)
        )

    def test_add_to_bucket_must_create_bucket_entries_in_cache(self):
        key = 'passport_work_time'
        value = 3.57

        self.golovan_aggregator = self.get_stats_aggregator(0.05, 10, 2)

        assert_that(
            self.golovan_aggregator.get_bucket_metric_names(),
            empty()
        )
        self.golovan_aggregator.add_to_bucket(key, value)

        assert_that(
            self.golovan_aggregator.get_bucket_metric_names(),
            has_item('passport_work_time_hgram')
        )
        assert_that(
            self.golovan_aggregator._cache_get('passport_work_time_hgram_6'),
            equal_to(1.0)
        )

    def test_build_buckets_stats(self):
        # Должны вернуть ключ и набор бакетов [0, 0] [0.5, 0] [1.0, 0] [1.5, 1]
        key = 'passport_work_time_hgram'

        experiments = [
            (-1, [(0, 1), (0.0078125, 0), (0.015625, 0), (0.03125, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 0),
                  (1.0, 0), (2.0, 0)]),
            (0.01, [(0, 0), (0.0078125, 1), (0.015625, 0), (0.03125, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 0),
                    (1.0, 0), (2.0, 0)]),
            (0.05, [(0, 0), (0.0078125, 0), (0.015625, 0), (0.03125, 1), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 0),
                    (1.0, 0), (2.0, 0)]),
            (1.57, [(0, 0), (0.0078125, 0), (0.015625, 0), (0.03125, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 0),
                    (1.0, 1), (2.0, 0)]),
            (0.57, [(0, 0), (0.0078125, 0), (0.015625, 0), (0.03125, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 1),
                    (1.0, 0), (2.0, 0)]),
            (0.79, [(0, 0), (0.0078125, 0), (0.015625, 0), (0.03125, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 1),
                    (1.0, 0), (2.0, 0)]),
            (0.8, [(0, 0), (0.0078125, 0), (0.015625, 0), (0.03125, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 1),
                   (1.0, 0), (2.0, 0)]),
            (2.90, [(0, 0), (0.0078125, 0), (0.015625, 0), (0.03125, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 0),
                    (1.0, 0), (2.0, 1)]),
        ]

        for value, exp_stats in experiments:
            golovan_aggregator = self.get_stats_aggregator(0.00625, 2.0, 2)
            golovan_aggregator.add_to_bucket(key, value)
            stats = golovan_aggregator._build_buckets_stats()
            assert_that(
                stats[0],
                equal_to((key, exp_stats))
            )

    def test_should_build_stats_with_cached_items(self):
        inc_400_metric_name = '400_count_summ'
        self.golovan_aggregator.inc(inc_400_metric_name, value=1)

        inc_500_metric_name = '500_count_summ'
        self.golovan_aggregator.inc(inc_500_metric_name, value=5)

        set_errors_metric_name = 'errors_count_summ'
        self.golovan_aggregator.set(set_errors_metric_name, value=12)

        result = self.golovan_aggregator.get_data()
        exp_result = [
            [set_errors_metric_name, 12.0],
            [inc_500_metric_name, 5.0],
            [inc_400_metric_name, 1.0],
        ]
        assert_that(len(result), equal_to(3))
        assert_that(result, has_items(*exp_result))

    def test_should_build_stats_with_cached_items_and_bucket(self):
        self.golovan_aggregator = self.get_stats_aggregator(0.05, 0.5, 2)

        inc_500_metric_name = '500_count_summ'
        self.golovan_aggregator.inc(inc_500_metric_name, value=5)

        bucket_key = 'response_time'
        second_bucket_key = 'database_response_time'
        self.golovan_aggregator.add_to_bucket(bucket_key, 0.27)
        self.golovan_aggregator.add_to_bucket(second_bucket_key, 0.07)

        result = self.golovan_aggregator.get_data()
        exp_result = [
            ['500_count_summ', 5.0],
            ('response_time_hgram', [(0, 0), (0.0625, 0), (0.125, 0), (0.25, 1), (0.5, 0), (1.0, 0)]),
            ('database_response_time_hgram', [(0, 0), (0.0625, 1), (0.125, 0), (0.25, 0), (0.5, 0), (1.0, 0)])
        ]
        assert_that(len(result), equal_to(3))
        assert_that(result, has_items(*exp_result))

    def test_aggregator_should_call_custom_metric_funcs(self):
        def custom_metric_func():
            return [['custom_metric_1_summ', 1], ['custom_metric_2_summ', 5]]

        def second_custom_metric_func():
            return [['second_custom_metric_1_summ', 10], ['second_custom_metric_2_summ', 15]]

        self.golovan_aggregator.add_metric_func(custom_metric_func)
        self.golovan_aggregator.add_metric_func(second_custom_metric_func)

        data = self.golovan_aggregator.get_data()

        exp_data = [
            ['custom_metric_1_summ', 1],
            ['custom_metric_2_summ', 5],
            ['second_custom_metric_1_summ', 10],
            ['second_custom_metric_2_summ', 15],
        ]

        assert_that(
            data,
            has_items(*exp_data)
        )

    def test_aggregator_custom_metrics_with_base_metrics(self):
        self.golovan_aggregator = self.get_stats_aggregator(0.05, 1.6, 2)

        bucket_key = 'some_work_time'
        custom_metrics = [['custom_metric_1_summ', 1], ['custom_metric_2_summ', 5]]

        def custom_metric_func():
            return custom_metrics

        self.golovan_aggregator.add_metric_func(custom_metric_func)

        self.golovan_aggregator.add_to_bucket(bucket_key, 1.51)

        data = self.golovan_aggregator.get_data()

        exp_data = [
            custom_metrics[0],
            custom_metrics[1],
            ('some_work_time_hgram', [(0, 0), (0.0625, 0), (0.125, 0), (0.25, 0), (0.5, 0), (1.0, 1), (2.0, 0)]),
        ]

        assert_that(data, has_items(*exp_data))

    def test_should_return_zero_for_num_metrics_if_there_is_no_value_in_cache(self):
        metric_name = 'errors_summ'
        value = 500

        self.golovan_aggregator.set(metric_name, value)
        assert_that(
            self.golovan_aggregator.get_num_metric_names(),
            contains(metric_name)
        )
        assert_that(
            self.golovan_aggregator._cache_get(metric_name),
            equal_to(value)
        )
        assert_that(
            self.golovan_aggregator.get_data(),
            has_items([metric_name, value])
        )

    def test_log_work_time(self):
        metric_name = 'some_key'
        with self.golovan_aggregator.log_work_time(metric_name):
            pass
        assert_that(
            self.golovan_aggregator.get_bucket_metric_names(),
            contains(
                '{}_work_time_hgram'.format(metric_name)
            )
        )

    def test_metric_name_validation(self):
        valid_names = [
            'some_metric_dmmm', 'some_metric_axxx', 'some_metric_avvv',
            'some_metric_summ', 'some_metric_hgram', 'some_metric_max',
            '_some_metric_attt', 'some-REALLY_long.sign@l/_dnnn',
            'tier=some_tier;_some_metric_summ', 'tag1=tag;tag2=tag2;some_metric_hgram',
        ]
        invalid_names = [
            'my_metric_admm', 'my_metric_admmm', 'my_metric_amm',
            '=metric_dmmm', 'some_metric_xmmm', 'some_metric_xxxx',
            'tier=some_tier;', 'tier=;some_metric_summ',
        ]

        for name in valid_names:
            self.golovan_aggregator.inc(name)
        for name in invalid_names:
            assert_that(calling(self.golovan_aggregator.inc).with_args(name), raises(ValueError))

