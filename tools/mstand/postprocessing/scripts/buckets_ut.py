import pytest

from postprocessing.scripts import buckets


# noinspection PyClassHasNoInit
class TestBuckets:
    def test_bucket(self):
        bucket = buckets.Bucket()
        bucket.add_value(1)
        bucket.add_value(2)
        bucket.add_value(3)

        assert bucket.sum == 6
        assert bucket.length == 3
        assert bucket.average() == 2.0

    def test_bucket_average_float(self):
        bucket = buckets.Bucket()
        bucket.add_value(1)
        bucket.add_value(2)

        assert bucket.sum == 3
        assert bucket.length == 2
        assert bucket.average() == 1.5

    def test_bucket_multiple_values(self):
        bucket = buckets.Bucket()
        bucket.add_values([1, 2, 3, 4, 5])

        assert bucket.sum == 15
        assert bucket.length == 5
        assert bucket.average() == 3.0

    def test_bucket_division(self):
        rows = [
            [1],
            [2],
            [3],
            [4],
            [5],
        ]

        postprocessor = buckets.BucketPostprocessor(num_buckets=1)
        result = postprocessor.do_buckets_division(rows)
        assert len(result) == 1

        bucket = result[0]
        assert bucket.sum == 15
        assert bucket.length == 5
        assert bucket.average() == 3.0

        postprocessor = buckets.BucketPostprocessor(num_buckets=2)
        bucket1, bucket2 = postprocessor.do_buckets_division(rows)
        assert 0 <= bucket1.length <= 5
        assert 0 <= bucket2.length <= 5
        assert 0 <= bucket1.sum <= 15
        assert 0 <= bucket2.sum <= 15

    def test_bucket_ratio(self):
        rows = [
            [6, 2],
            [6, 4],
        ]

        postprocessor = buckets.BucketPostprocessor(num_buckets=1, aggregate_by="ratio")
        bucket, = postprocessor.do_buckets_division(rows)
        assert bucket.sum == 12
        assert bucket.length == 6
        bucket_value, = postprocessor.collect_bucket_results([bucket])
        assert bucket_value == 2

    def test_collect_bucket_values(self):
        test_buckets = [
            buckets.Bucket.from_values([1, 2, 3]),
            buckets.Bucket.from_values([4, 5, 6]),
            buckets.Bucket.from_values([7]),
        ]

        postprocessor = buckets.BucketPostprocessor(num_buckets=3)
        averages = list(postprocessor.collect_bucket_results(test_buckets))
        assert averages == [2, 5, 7]

        postprocessor = buckets.BucketPostprocessor(num_buckets=3, aggregate_by="sum")
        sums = list(postprocessor.collect_bucket_results(test_buckets))
        assert sums == [6, 15, 7]

    def test_empty_buckets(self):
        test_buckets = [
            buckets.Bucket.from_values([1, 2, 3]),
            buckets.Bucket.from_values([4, 5, 6]),
            buckets.Bucket.from_values([]),
        ]

        postprocessor = buckets.BucketPostprocessor(num_buckets=1)
        with pytest.raises(Exception):
            list(postprocessor.collect_bucket_results(test_buckets))

        postprocessor = buckets.BucketPostprocessor(num_buckets=1, skip_experiments_with_emtpy_buckets=True)
        assert list(postprocessor.collect_bucket_results(test_buckets)) == []
