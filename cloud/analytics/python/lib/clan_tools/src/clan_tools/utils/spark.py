import logging
from functools import wraps
from typing import Dict, Tuple, Callable, Any, TypeVar, cast
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter

logger = logging.getLogger(__name__)


Function = TypeVar('Function', bound=Callable[..., Any])


_SPARK_CONF = {
    "spark.executor.memory": "6G",
    "spark.executor.cores": 2,
    "spark.sql.session.timeZone": "UTC",
    "spark.sql.autoBroadcastJoinThreshold": -1,
    "spark.driver.memory": "4G",
}


SPARK_CONF_SMALL = {
    **_SPARK_CONF,
    **{
        "spark.cores.max": 8,
        "spark.dynamicAllocation.maxExecutors": 4
    }
}

SPARK_CONF_MEDIUM = {
    **_SPARK_CONF,
    **{
        "spark.cores.max": 12,
        "spark.dynamicAllocation.maxExecutors": 6
    }
}

SPARK_CONF_LARGE = {
    **_SPARK_CONF,
    **{
        "spark.cores.max": 16,
        "spark.dynamicAllocation.maxExecutors": 8
    }
}


DEFAULT_SPARK_CONF = SPARK_CONF_MEDIUM


SPARK_CONF_GRAPHS = {
    **DEFAULT_SPARK_CONF,
    **{
        "spark.jars": 'yt:///home/sashbel/graphframes-assembly-0.8.2-SNAPSHOT-spark3.0.jar'
    }
}


def prepare_for_yt(func: Function) -> Function:
    @wraps(func)
    def wrapper(*args: Tuple[Any, ...], **kwargs: Dict[str, Any]) -> Any:

        df = func(*args, **kwargs)
        for name, typ in df.dtypes:
            if typ == "timestamp":
                df = df.withColumn(name, col(name).cast("long"))
            elif typ == "date":
                df = df.withColumn(
                    name, F.to_timestamp(col(name)).cast("long"))
            elif typ.startswith("decimal"):
                df = df.withColumn(name, col(name).cast("double"))
        return df

    return cast(Function, wrapper)


def safe_append_spark(yt_adapter: YTAdapter, spdf: SparkDataFrame, path: str, mins_to_wait: float = 15.0) -> None:
    """Appends `spdf` to table in `path` with accurately pre-taken `shared lock`.
    This algorithm helps to avoid errors caused by in-time `exclusive lock` state of target table.

    :param yt_adapter: Object of `YTAdapter` class
    :param spdf: SparkDataFrame to be saved
    :param path: Path on YT to be saved in
    :param path: Amount of minutes to wait on getting `shared lock` if it is unavailable in a moment
    """
    if yt_adapter.yt.exists(path):
        ms_to_wait = int(mins_to_wait * 60 * 1000)  # converting mins to milliseconds
        with yt_adapter.yt.Transaction():  # creating transaction to take `shared lock`
            yt_adapter.yt.lock(path=path, mode='shared', waitable=True, wait_for=ms_to_wait)
            spdf.write.yt(path, mode='append')
    else:
        spdf.write.yt(path)

__all__ = ['SPARK_CONF_SMALL', 'SPARK_CONF_MEDIUM', 'SPARK_CONF_LARGE', 'DEFAULT_SPARK_CONF', 'SPARK_CONF_GRAPHS', 'prepare_for_yt', 'safe_append_spark']
