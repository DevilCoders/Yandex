import typing as tp
import warnings

from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter
from ml_flows.features.BaseFeatureAdapter import BaseFeatureAdapter
from ml_flows.features.PaymentsFeaturesAdapter import PaymentsFeaturesAdapter
from ml_flows.features.ConsumptionFeaturesAdapter import ConsumptionFeaturesAdapter
from ml_flows.features.CryptaFeaturesAdapter import CryptaFeaturesAdapter
from ml_flows.features.MetrikaVisitsFeaturesAdapter import MetrikaVisitsFeaturesAdapter
from ml_flows.features.MetrikaHitsFeaturesAdapter import MetrikaHitsFeaturesAdapter


class ConfigLoader:

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter, spdf_target: SparkDataFrame, date_col: str = 'date', verbose: bool = True) -> None:
        self._spark = spark
        self._yt_adapter = yt_adapter
        self._spdf_target = spdf_target
        self._date_col = date_col
        self._verbose = verbose

    def load_config(self, cfg: tp.Dict[str, tp.Dict[str, tp.Dict[str, tp.List[str]]]]) -> SparkDataFrame:

        result_spdf: tp.Optional[SparkDataFrame] = None
        for period in cfg.keys():
            for source in cfg[period].keys():

                if self._verbose:
                    print(f'Preparing {period} - {source}...')

                if source == 'consumption':
                    data_source: BaseFeatureAdapter = ConsumptionFeaturesAdapter(
                        self._spark, self._yt_adapter, self._spdf_target, self._date_col, cfg[period][source], int(period))

                elif source == 'payments':
                    data_source = PaymentsFeaturesAdapter(
                        self._spark, self._yt_adapter, self._spdf_target, self._date_col, cfg[period][source], int(period))

                elif source == 'crypta':
                    data_source = CryptaFeaturesAdapter(
                        self._spark, self._yt_adapter, self._spdf_target, self._date_col, cfg[period][source], int(period))

                elif source == 'visits':
                    data_source = MetrikaVisitsFeaturesAdapter(
                        self._spark, self._yt_adapter, self._spdf_target, self._date_col, cfg[period][source], int(period))

                elif source == 'hits':
                    data_source = MetrikaHitsFeaturesAdapter(
                        self._spark, self._yt_adapter, self._spdf_target, self._date_col, cfg[period][source], int(period))

                else:
                    warnings.warn(f'Unknown source {source}', SyntaxWarning)
                    continue

                if result_spdf is None:
                    result_spdf = data_source.get_double_features()
                else:
                    result_spdf = (
                        result_spdf
                        .join(
                            data_source.get_double_features(),
                            on=['billing_account_id', self._date_col],
                            how='left'
                        )
                    )

        return result_spdf
