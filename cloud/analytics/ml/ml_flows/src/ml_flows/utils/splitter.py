import typing as tp

import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

ID_RANDOM_VALUE_COLUMN = 'RVAL_COLUMN'
DT_NUMBER_VALUE_COLUMN = 'NVAL_COLUMN'


class SparkDataSplitter:

    def __init__(self, spark: SparkSession, id_colname: tp.Optional[str] = None,
                 date_colname: tp.Optional[str] = None, target_colname: tp.Optional[str] = None,
                 random_state: int = 42, id_split_ratio: float = 0.7, date_split_ratio: float = 0.7) -> None:
        self._spark = spark
        self._random_state = random_state
        self._id_colname = id_colname
        self._date_colname = date_colname
        self._target_colname = target_colname
        self._id_split_ratio = id_split_ratio
        self._date_split_ratio = date_split_ratio

    def split(self, spdf: SparkDataFrame) -> tp.Tuple[SparkDataFrame, SparkDataFrame]:
        appropriate_split = self._get_splitter(spdf)
        return appropriate_split(spdf)

    def _get_splitter(self, spdf: SparkDataFrame) -> tp.Callable[[SparkDataFrame], tp.Tuple[SparkDataFrame, SparkDataFrame]]:
        if self._id_colname is None:
            if self._date_colname is None:
                raise AttributeError('Define at least one of "id_colname" or "date_colname"')
            else:
                return self._date_split
        else:
            if self._date_colname is None:
                if self._target_colname is None:
                    return self._id_random_split
                else:
                    if spdf.select(self._target_colname).distinct().agg(F.count('*')).collect()[0][0] == 2:
                        return self._id_stratified_split
                    else:
                        return self._id_random_split
            else:
                if self._target_colname is None:
                    return self._id_date_random_split
                else:
                    if spdf.select(self._target_colname).distinct().agg(F.count('*')).collect()[0][0] == 2:
                        return self._id_date_stratified_split
                    else:
                        return self._id_date_random_split

    def _id_random_split(self, spdf: SparkDataFrame) -> tp.Tuple[SparkDataFrame, SparkDataFrame]:
        """self._id_colname - должен быть определен"""
        spdf_unique_id_set = (
            spdf
            .select(self._id_colname)
            .distinct()
            .select(
                self._id_colname,
                F.rand(seed=self._random_state).alias(ID_RANDOM_VALUE_COLUMN)
            )
            .cache()
        )

        spdf_train_id_set = (
            spdf_unique_id_set
            .filter(col(ID_RANDOM_VALUE_COLUMN) <= self._id_split_ratio)
            .select(self._id_colname)
        )
        spdf_test_id_set = (
            spdf_unique_id_set
            .filter(col(ID_RANDOM_VALUE_COLUMN) > self._id_split_ratio)
            .select(self._id_colname)
        )

        spdf_train = spdf.join(spdf_train_id_set, on=self._id_colname, how='inner')
        spdf_test = spdf.join(spdf_test_id_set, on=self._id_colname, how='inner')
        return spdf_train, spdf_test

    def _id_stratified_split(self, spdf: SparkDataFrame) -> tp.Tuple[SparkDataFrame, SparkDataFrame]:
        """self._id_colname self._target_colname - должны быть определены"""
        target_pos, target_neg = (
            spdf
            .select(self._target_colname)
            .distinct()
            .toPandas()[self._target_colname]
            .tolist()
        )

        spdf_pos = spdf.filter(col(self._target_colname) == target_pos)
        spdf_neg = spdf.filter(col(self._target_colname) == target_neg)

        spdf_pos_train, spdf_pos_test = self._id_random_split(spdf_pos)
        spdf_neg_train, spdf_neg_test = self._id_random_split(spdf_neg)

        spdf_train = spdf_pos_train.union(spdf_neg_train).orderBy(F.rand())
        spdf_test = spdf_pos_test.union(spdf_neg_test).orderBy(F.rand())

        return spdf_train, spdf_test

    def _date_split(self, spdf: SparkDataFrame) -> tp.Tuple[SparkDataFrame, SparkDataFrame]:
        """self._date_colname - должен быть определен"""
        df_date_stats = (
            spdf
            .select(
                F.unix_timestamp(col(self._date_colname).cast('timestamp')).alias(DT_NUMBER_VALUE_COLUMN)
            )
            .agg(
                F.min(DT_NUMBER_VALUE_COLUMN).alias('min_dt'),
                F.max(DT_NUMBER_VALUE_COLUMN).alias('max_dt')
            )
            .toPandas()
        )
        min_dt = df_date_stats['min_dt'][0]
        max_dt = df_date_stats['max_dt'][0]
        border_dt = int(min_dt + (max_dt - min_dt) * self._date_split_ratio)

        spdf_train = spdf.filter(F.unix_timestamp(col(self._date_colname).cast('timestamp')) <= lit(border_dt))
        spdf_test = spdf.filter(F.unix_timestamp(col(self._date_colname).cast('timestamp')) > lit(border_dt))

        return spdf_train, spdf_test

    def _id_date_random_split(self, spdf: SparkDataFrame) -> tp.Tuple[SparkDataFrame, SparkDataFrame]:
        """self._id_colname self._date_colname self._target_colname - должны быть определены"""
        spdf_date_train, spdf_date_test = self._date_split(spdf)
        spdf_id_train, spdf_id_test = self._id_random_split(spdf)

        spdf_train = spdf_date_train.join(spdf_id_train, on=spdf.columns, how='inner')
        spdf_test = spdf_date_test.join(spdf_id_test, on=spdf.columns, how='inner')

        return spdf_train, spdf_test

    def _id_date_stratified_split(self, spdf: SparkDataFrame) -> tp.Tuple[SparkDataFrame, SparkDataFrame]:
        """self._id_colname self._date_colname self._target_colname - должны быть определены"""
        spdf_date_train, spdf_date_test = self._date_split(spdf)
        spdf_id_train, spdf_id_test = self._id_stratified_split(spdf)

        spdf_train = spdf_date_train.join(spdf_id_train, on=spdf.columns, how='inner')
        spdf_test = spdf_date_test.join(spdf_id_test, on=spdf.columns, how='inner')

        return spdf_train, spdf_test
