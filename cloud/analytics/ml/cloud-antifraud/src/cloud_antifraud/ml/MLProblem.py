import logging
from collections import Counter
from dataclasses import dataclass
from typing import List

import numpy as np
import pandas as pd

from sklearn.compose import ColumnTransformer
from sklearn.impute import SimpleImputer
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import MinMaxScaler, OneHotEncoder, RobustScaler, StandardScaler

from clan_tools.utils.cache import cached
from clan_tools.utils.timing import timing

logger = logging.getLogger(__name__)


@dataclass
class ProblemDescription:
    cat_features: List[str]
    cat_features_idx: List[int]
    num_features: List[str]
    num_features_idx: List[int]



@dataclass
class MLProblem:
    X: pd.DataFrame
    y: pd.Series
    features: pd.DataFrame
    description: ProblemDescription
    preprocessor: ColumnTransformer


class MLProblemCreator:
    def __init__(self, exclude_columns: List[str], target_column: str):
        self._exclude_cols = exclude_columns
        self._target_column = target_column

    @timing
    # @cached('data/cache/get_problem.pkl')
    def get_problem(self, features: pd.DataFrame):
        X = features.drop(self._exclude_cols, axis=1)
        unique_values_count = X.apply(lambda col: col.nunique(), axis=0)
        bin_cat = unique_values_count[unique_values_count <= 3].index.tolist()
        X[bin_cat] = X[bin_cat].astype(int)
        string_cat = X.dtypes[X.dtypes == object].index.tolist()
        X[string_cat] = X[string_cat].astype(str)
        categorical = bin_cat + string_cat
        num_features = X.columns[~X.columns.isin(categorical)].tolist()
        x_cols = num_features + bin_cat + string_cat
        X = X[x_cols]
        categorical_transformer = OneHotEncoder(handle_unknown='ignore')
        preprocessor = ColumnTransformer(
            transformers=[
                ('num', RobustScaler(), num_features),
                ('bin', MinMaxScaler(), bin_cat),

                #         ('cat', categorical_transformer, string_cat),

            ], remainder='passthrough')
        X = preprocessor.fit_transform(X)
        unq, uidx, count = np.unique(
            X, axis=0, return_index=True, return_counts=True)
        features = features.iloc[uidx, :]
        X = pd.DataFrame(X[uidx, :], columns=x_cols)
        y = features[self._target_column].astype(int)

        y[(y == 1) & ((features.total_balance > 0)
                      | (features.is_verified == 1) 
                      | (features.unblock_reason == 'manual')  
                      | (features.ba_state == 'active'))] = -1

        y[(y == 0)  
             & (~features.is_verified)  
             & (features.yandex_staff == 0)] = -1
        
            #   | ((features.ba_time*1000).apply(utcms2datetime) < datetime(2020, 1, 1))

        y[features.is_verified == 1] = 0
        y[features.yandex_staff == 1] = 0
        y[features.sales_name != 'unmanaged'] = 0

        logger.info(f'Counter(y) = {Counter(y)}')
        return MLProblem(X, y, features, preprocessor = preprocessor, description=
                    ProblemDescription(cat_features= bin_cat,
                                       cat_features_idx = list(range(len(num_features), len(x_cols))),
                                       num_features=num_features,  
                                       num_features_idx= list(range(len(num_features))) ))
