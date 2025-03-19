import numpy as np  # type: ignore
import pandas as pd  # type: ignore
import statsmodels.api as sm  # type: ignore
import logging.config

from datetime import datetime, timedelta
from dataclasses import dataclass
from itertools import product
from sklearn.linear_model import Lasso  # type: ignore
from sklearn.preprocessing import OneHotEncoder  # type: ignore
from typing import Optional, Dict, List, Tuple, Union, Any
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@dataclass
class SarimaxParams:
    """Class for keeping Sarimax params in inventory"""
    p: int = 0
    d: int = 0
    q: int = 0
    P: int = 0
    D: int = 0
    Q: int = 0
    S: int = 0
    trend: Optional[str] = None

    def __post_init__(self):
        num_args = (self.p, self.d, self.q, self.P, self.D, self.Q, self.S)
        assert all([p >= 0 and p % 1 == 0 for p in num_args])
        assert self.trend in (None, 'n', 'c', 't', 'ct')
        self.p, self.d, self.q = int(self.p), int(self.d), int(self.q)
        self.P, self.D, self.Q, self.S = int(self.P), int(self.D), int(self.Q), int(self.S)

    def kwargs(self) -> Dict[str, Any]:
        return {
            'order': (self.p, self.d, self.q),
            'seasonal_order': (self.P, self.D, self.Q, self.S),
            'trend': self.trend
        }

    def to_dict(self) -> Dict[str, Union[int, Optional[str]]]:
        """Returns simple dict"""
        return {
            'p': self.p, 'd': self.d, 'q': self.q,
            'P': self.P, 'D': self.D, 'Q': self.Q, 'S': self.S,
            'trend': self.trend
        }

    def weight(self) -> int:
        """Weight - some metric to describe complexity of model with that params
        It is linear for that case
        """
        trend_weight = {'n': 0, 'c': 2, 't': 4, 'ct': 6}
        return trend_weight[self.trend] + sum(self.p, self.d, self.q, self.P, self.D, self.Q)

    @classmethod
    def from_dict(cls, dd: Dict[str, Any]) -> 'SarimaxParams':
        """Creates **kwargs for SARIMAX model"""
        int_keys = ['p', 'd', 'q', 'P', 'D', 'Q', 'S']
        kwargs = {key: dd.get(key, 0) for key in int_keys}
        kwargs.update({'trend': dd.get('trend', 'ct')})
        return cls(**kwargs)


def root_mean_squared_logarithmic_error(y_true, y_pred, a_min=1.) -> float:
    """Target metric for that module"""
    if any(y_true <= 0):
        raise ValueError('y_true can not be negative')
    squared_log_diff = (np.log(np.maximum(y_pred, a_min)) - np.log(y_true))**2
    return np.sqrt(np.mean(squared_log_diff))


class TimeSeriesForecast:
    """Class for making forecast
    It might be set of separate functions but they are integrated in one class due to they all have common design
    """
    def __init__(self, target_col: str, date_col: str,
                 cfg: Optional[Dict[str, Any]] = None,
                 filter_colnames: Optional[List] = None):
        self.target_col = target_col
        self.date_col = date_col
        self.f_colnames = filter_colnames or []

    def prepare_sample(self, df: pd.DataFrame, h_days: int = 730, f_days: int = 730) -> Tuple[pd.Series, pd.DataFrame, pd.DataFrame]:
        """Creates endog, exog and exog_fut frames.

        Exogenous data - data about calendar data. It allow us to ignore seasonal components in SARIMA
        and concentrate only on autoregressive components.
        Combination of data might be mpdified in future. But that data must be inline with requirement to be known in advance.

        Args:
            df (pd.DataFrame): input data with required columns self.target_col and self.date_col
            h_days (int): maximum days in history
            f_days (int): days in future exogenous data

        Returns:
            endog (pd.Series): endogenous timeseries - target series to be fitted and predicted in ARIMA
            exog (pd.DataFrame): exogenous timeseries - additional features that could be used in model
            exog_fut (pd.DataFrame): same as exog, but in future time - it might be used in inference
        """
        if df.shape[0] <= 30:
            endog = pd.Series(name=self.target_col)
            exog_start = datetime.strptime(datetime.now().strftime('%Y-%m-%d'), '%Y-%m-%d')
            exog_end = exog_start
        else:
            ts = df[[self.target_col]].set_index(pd.to_datetime(df[self.date_col]))[self.target_col]

            assert any(ts.resample('1D').count() <= 1), "Need to have maximum 1 observation during the day"
            assert h_days > 0 and f_days > 0, "'f_days' and 'h_days' arguments must be positive"

            ts = ts.resample('1D').last().ffill().iloc[-h_days:]
            endog = ts.copy()
            exog_start = ts.index[0]
            exog_end = ts.index[-1]

        exog_fut_end = exog_end + timedelta(f_days)

        df_exog = pd.DataFrame(index=pd.date_range(exog_start, exog_fut_end))
        exog_feats = pd.DataFrame({'dw': df_exog.index.day_of_week, 'dm': df_exog.index.day})
        enc_exog_feats = OneHotEncoder(handle_unknown='ignore', sparse=False)
        df_exog.loc[:, enc_exog_feats.get_feature_names()] = enc_exog_feats.fit_transform(exog_feats)
        df_exog['is_last_day'] = (df_exog.index.day == df_exog.index.days_in_month) * 1.

        exog = df_exog[df_exog.index <= exog_end]
        exog_fut = df_exog[df_exog.index > exog_end]

        return endog, exog, exog_fut

    def make_const_prediction(self, endog: pd.Series, exog: pd.DataFrame, exog_fut: pd.DataFrame, p_val: float = 0.01) -> pd.DataFrame:
        """Makes simple constant prediction"""
        if len(endog) <= 30:
            return self.make_nan_prediction(endog, exog, exog_fut)
        res = pd.DataFrame(index=exog_fut.index)
        res['mean'] = int(endog.median())
        res['mean_se'] = round(endog.std(), 2)
        res['mean_ci_lower'] = max(int(round(endog.quantile(q=(p_val / 2)) - 1)), 0)
        res['mean_ci_upper'] = int(round(endog.quantile(q=(1 - p_val / 2)) + 1))

        return res

    def make_nan_prediction(self, endog: pd.Series, exog: pd.DataFrame, exog_fut: pd.DataFrame) -> pd.DataFrame:
        """Fill inference data with np.nan"""
        res = pd.DataFrame(index=exog_fut.index)
        res['mean'] = np.nan
        res['mean_se'] = np.nan
        res['mean_ci_lower'] = np.nan
        res['mean_ci_upper'] = np.nan

        return res

    def _find_features_with_impact(self, endog: pd.Series, exog: pd.DataFrame, test_ratio: float = 0.25) -> List:
        """Fast and rough feature selection through L1-regularization.
        Works along with log-grid-search of regularization coef providing good enough feature-selection
        for linear models.
        """
        exog = exog.copy()
        exog['tech1'] = np.arange(exog.shape[0])
        exog['tech2'] = np.arange(exog.shape[0])**2

        Y_train, Y_test = endog[:-int(len(endog)*test_ratio)], endog[-int(len(endog)*test_ratio):]
        X_train, X_test = exog[:-int(len(endog)*test_ratio)], exog[-int(len(endog)*test_ratio):]

        min_score_alpha = np.nan
        min_score = np.inf
        for alpha in np.logspace(-5, 10, num=80):
            model = Lasso(alpha=alpha)
            model.fit(X_train, Y_train)
            curr_score = root_mean_squared_logarithmic_error(Y_test, model.predict(X_test))

            if curr_score <= min_score:
                min_score_alpha = alpha
                min_score = curr_score

        model = Lasso(alpha=min_score_alpha)
        model.fit(X_train, Y_train)
        collist = X_train.columns[np.abs(model.coef_) > 1e-3].tolist()
        if 'tech1' in collist:
            collist.remove('tech1')
        if 'tech2' in collist:
            collist.remove('tech2')
        return collist

    def make_sarimax_prediction(self, endog: pd.Series, exog: pd.DataFrame, exog_fut: pd.DataFrame, colnames: Optional[List] = None,
                                param: 'SarimaxParams' = SarimaxParams(1, 1, 1, trend='ct')) -> pd.DataFrame:
        """Makes Sarimax prediction with a given params"""
        if len(endog) <= 30:
            return self.make_nan_prediction(endog, exog, exog_fut)
        if colnames is None:
            colnames = self._find_features_with_impact(endog, exog)

        if len(colnames) > 0:
            X = exog[colnames]
            X_fut = exog_fut[colnames]
        else:
            X, X_fut = None, None
        try:
            sarimax = sm.tsa.SARIMAX(endog, X, **param.kwargs()).fit(maxiter=200)
            res = sarimax.get_forecast(exog_fut.index[-1], exog=X_fut).summary_frame().clip(lower=0)
        except Exception:
            res = self.make_nan_prediction(endog, exog, exog_fut)

        return res

    def optimize_hyperparams(self, endog: pd.Series, exog: pd.DataFrame, test_ratio: float = 0.25,
                             alpha: float = 0.001) -> Tuple['SarimaxParams', List[str], List[Tuple[Union[float, int, Optional[str]]]]]:
        """Special function with full training process. It makes feature-selection and hyperparams-selection
        """
        # datasets
        colnames = self._find_features_with_impact(endog, exog)
        logger.info('colnames:', colnames)
        Y_train, Y_test = endog[:-int(len(endog)*test_ratio)], endog[-int(len(endog)*test_ratio):]
        if len(colnames) > 0:
            X_train = exog[:-int(len(endog)*test_ratio)][colnames]
            X_test = exog[-int(len(endog)*test_ratio):][colnames]
        else:
            X_train = None
            X_test = None
        to_date = Y_test.index[-1]

        # universal parameters list
        ps = np.arange(6)
        ds = np.arange(2)
        qs = np.arange(6)
        parameters_list = product(ps, ds, qs, [0], [0], [0], [0], ['ct'])

        min_score_params = None
        min_score = np.inf
        log = []
        for param in parameters_list:
            p = SarimaxParams(*param)

            try:
                model = sm.tsa.SARIMAX(Y_train, X_train, **p.kwargs()).fit(maxiter=200)
                Y_pred = model.get_forecast(to_date, exog=X_test).summary_frame()['mean']
            except Exception:
                logger.info(p, 'ignored')
                continue

            curr_score = root_mean_squared_logarithmic_error(Y_test, Y_pred) + alpha * p.weight()
            log.append((curr_score, *param))
            if curr_score <= min_score:
                min_score_params = p
                min_score = curr_score

        logger.info('params:', min_score_params)
        logger.info('score:', min_score)
        return min_score_params, colnames, log


__all__ = ['SarimaxParams', 'root_mean_squared_logarithmic_error', 'TimeSeriesForecast']
