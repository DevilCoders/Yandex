import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql import DataFrame
from typing import Iterable

config = {
    "weights": [
        "event_exp_7d_half_life_time_decay_weight",
        "event_first_weight",
        "event_last_weight",
        "event_u_shape_weight",
        "event_uniform_weight"
    ],
    "market_channels": [
        "Recommender Systems",
        "Social (organic)",
        "Other",
        "Yandex Portal",
        "Events",
        "Messengers",
        "Emailing",
        "Referrals",
        "Social Owned",
        "Site Others",
        "YC App",
        "Organic Search",
        "Performance",
        "Unknown",
        "Site and Console Promo",
        "Direct"
    ],
    "segments": ["Mass"],
    "start_date": "2019-01-01",
    "end_date": "2022-01-01"
}


def spark_melt(
        df: DataFrame,
        id_vars: Iterable[str], value_vars: Iterable[str],
        var_name: str="variable", value_name: str="value") -> DataFrame:

    _vars_and_vals = F.array(*(
        F.struct(F.lit(c).alias(var_name), col(c).alias(value_name))
        for c in value_vars))
    _tmp = df.withColumn("_vars_and_vals", F.explode(_vars_and_vals))

    cols = id_vars + [col("_vars_and_vals")[x].alias(x) for x in [var_name, value_name]]
    return _tmp.select(*cols)