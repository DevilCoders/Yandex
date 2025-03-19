import typing as tp
import numpy as np

import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.column import Column as SparkColumn


def min_by(colname_min: str, colname_by: str) -> SparkColumn:
    return F.expr(f'Min_By(`{colname_min}`, `{colname_by}`)')


def max_by(colname_max: str, colname_by: str) -> SparkColumn:
    return F.expr(f'Max_By(`{colname_max}`, `{colname_by}`)')


def slope(values: tp.Union[str, SparkColumn], dates: tp.Union[str, SparkColumn]) -> SparkColumn:

    def get_slope_udf_before(obj: tp.List[np.ndarray]) -> tp.Optional[float]:  # type: ignore

        if len(obj) <= 3:
            return None
        else:
            y = np.ravel([elem[0] for elem in sorted(obj, key=lambda x: x[1])]).astype(np.float64)
            x = np.arange(len(y))

            coeffs = np.polyfit(x, y, 1)
            slope = coeffs[-2]
            return float(slope)

    get_slope_obj = F.udf(get_slope_udf_before, returnType=T.DoubleType())

    return get_slope_obj(F.collect_list(F.array(values, dates)))
