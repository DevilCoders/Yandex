import logging
import numbers
import random

from yaqutils import YaqEnum


class Bucket(object):
    def __init__(self):
        self.sum = 0
        self.length = 0

    def average(self):
        return float(self.sum) / self.length

    def add_value(self, value, denominator=1):
        self.sum += value
        self.length += denominator

    def add_values(self, values):
        for value in values:
            self.add_value(value)

    @staticmethod
    def from_values(values):
        bucket = Bucket()
        bucket.add_values(values)
        return bucket


class BucketModes(YaqEnum):
    SUM = "sum"
    AVERAGE = "average"
    RATIO = "ratio"

    ALL = [SUM, AVERAGE, RATIO]


class BucketPostprocessor(object):
    def __init__(self, num_buckets=100, aggregate_by="average", flatten_array=False,
                 skip_experiments_with_emtpy_buckets=False):
        """
        :type num_buckets: int
        :type aggregate_by: str
        :type flatten_array: bool
        :type skip_experiments_with_emtpy_buckets: bool
        """
        if num_buckets < 1:
            raise Exception("Buckets number should be at least 1")
        self.num_buckets = num_buckets
        self.mode = aggregate_by
        self.flatten_array = flatten_array
        self.skip_experiments_with_emtpy_buckets = skip_experiments_with_emtpy_buckets
        logging.info(
            "Bucket params: num_buckets = %s, mode = %s, flatten = %s, skip_experiments_with_emtpy_buckets = %s",
            num_buckets,
            aggregate_by,
            flatten_array,
            skip_experiments_with_emtpy_buckets,
        )
        assert self.mode in BucketModes.ALL, "Strange mode: {} not in {}".format(self.mode, BucketModes.ALL)

    def name(self):
        if self.mode == BucketModes.SUM:
            return "Buckets-sum({})".format(self.num_buckets)
        elif self.mode == BucketModes.AVERAGE:
            return "Buckets-avg({})".format(self.num_buckets)
        elif self.mode == BucketModes.RATIO:
            return "Buckets-ratio({})".format(self.num_buckets)
        assert False, "Strange mode: {} not in {}".format(self.mode, BucketModes.ALL)

    def do_buckets_division(self, exp):
        """
        :type exp: collections.Iterable[list[float]]
        :rtype: list[Bucket]
        """
        buckets = [Bucket() for _ in range(self.num_buckets)]
        exp_rows = 0
        for row in exp:
            exp_rows += 1
            bucket = random.choice(buckets)

            if self.mode == BucketModes.RATIO:
                if len(row) != 2:
                    raise Exception(
                        "In 'ratio' mode, we expect exactly 2 values in each row: numerator and denominator.")
                bucket.add_value(row[0], row[1])
                continue

            # this handles case when metric returned ONE value which is array of numbers.
            # in flatten_array mode, we "unwind" this array to separate numbers.
            # request of leratsoy@: flatten mode should work in BOTH cases:
            # 1) row = [[1,2,3]], i.e TSV-line='[1,2,3]'
            # 2) row = [1,2,3], i.e TSV-line='1\t2\t3'
            if self.flatten_array:
                if isinstance(row[0], numbers.Number):
                    bucket.add_values(row)
                elif len(row) == 1 and isinstance(row[0], list):
                    bucket.add_values(row[0])
                else:
                    raise Exception("flatten_array mode failed: your row is neither '[[1,2,3]]', nor '1<tab>2<tab>3'.")
            else:
                bucket.add_values(row)
        logging.info("Total rows processed: %s", exp_rows)
        return buckets

    def collect_bucket_results(self, buckets):
        """
        :type buckets: list[Bucket]
        :rtype: collections.Iterable[float]
        """
        if any(bucket.length == 0 for bucket in buckets):
            if self.skip_experiments_with_emtpy_buckets:
                return  # ignore all buckets MSTAND-1290
            raise Exception("Empty buckets!")

        for bucket in buckets:
            assert bucket.length > 0
            if self.mode == BucketModes.SUM:
                bucket_value = bucket.sum
            else:
                bucket_value = bucket.average()
            yield bucket_value

    def process_experiment(self, exp):
        """
        :type exp: ExperimentForPostprocessAPI
        :rtype:
        """
        logging.info("Starting bucket division")
        buckets = self.do_buckets_division(exp)
        logging.info("Collecting bucket results")
        bucket_values = self.collect_bucket_results(buckets)
        for value in bucket_values:
            exp.write_value(value)
