import numpy as np
import pandas as pd
from sklearn.pipeline import Pipeline
from sklearn.base import BaseEstimator, RegressorMixin, TransformerMixin
from statsmodels.discrete.discrete_model import Logit
from statsmodels.stats.outliers_influence import variance_inflation_factor
from sklearn.metrics import precision_score, recall_score, f1_score, roc_auc_score
from functools import partial


def metric_wrapper(metric, y_true, y_pred, th):
    return metric(y_true, y_pred > th)


def gini(y_true, y_pred, th):
    return 2 * roc_auc_score(y_true, y_pred > th) - 1


def frac(y_true, y_pred, th):
    return (y_pred > th).mean()


metrics = {
    'Frac' : frac,
    'Precision' : partial(metric_wrapper, metric=precision_score),
    'Recall' : partial(metric_wrapper, metric=recall_score),
    'F1' : partial(metric_wrapper, metric=f1_score),
    'Gini' : gini
    }


numerical_features = [
    'balance_float', 'autopay_failures_count',
    'payment_type', 'person_type', 'days_from_created',
    'days_after_last_payment', 'days_after_became_paid', 'bnpl_score'
]

training_cols = [
    'autopay_failures_count', 'balance_float', 'bnpl_score',
    'days_after_became_paid', 'days_from_created', 'is_isv', 
    'is_var', 'payment_type', 'person_type', 'days_after_last_payment'
]


class Preprocessor(BaseEstimator, TransformerMixin):

    def __init__(self):
        self.bnpl_score_mean = 1

    def fit(self, X, y=None):
        X = X.copy()
        self.bnpl_score_mean = X['bnpl_score'].mean()
        return self

    def transform(self, X, y=None):
        X = X.copy()
        X['balance_float'] = X['balance_float'].fillna(0)
        X['days_after_last_payment'] = X['days_after_last_payment'].fillna(1000)
        X['bnpl_score'] = X['bnpl_score'].fillna(self.bnpl_score_mean)
        X = X.replace([np.inf, -np.inf], 1).fillna(0)
        return X


class LogRegressionWrapper(BaseEstimator, RegressorMixin):

    def fit(self, X, y):
        self.model_ = Logit(y, X)
        self.results_ = self.model_.fit()
        return self

    def predict(self, X):
        return self.results_.predict(X)


class ModelPipeline(Pipeline):

    def __init__(self, steps, memory=None, verbose=False):
        super().__init__(steps, memory=memory, verbose=verbose)

    @staticmethod
    def get_vif(X):
        exog = X[training_cols]
        vif = pd.Series([variance_inflation_factor(exog.values, i) for i in range(exog.shape[1])], index=exog.columns)
        return vif

    @staticmethod
    def get_metrics(y_true, y_pred):
        gini_all = 2 * roc_auc_score(y_true, y_pred) - 1
        nibs = np.linspace(0, 1, 11)
        mics = pd.DataFrame(columns=nibs, index=list(metrics.keys()))
        for th in nibs:
            for name in metrics:
                mics.loc[name, th] = metrics[name](y_true, y_pred, th)
        mics.loc['Gini_all', 0] = gini_all
        return mics
