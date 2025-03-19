from clan_tools.data_adapters.YTAdapter import YTAdapter
import pandas as pd
from datetime import datetime
import numpy as np
import logging

logger = logging.getLogger(__name__)


class PredictionsAdapter:
    def __init__(self, yt_adapter: YTAdapter, result_path: str):
        self._yt_adapter = yt_adapter
        self._result_path = result_path

    def _prepare_prediction(self, clf, predict_features, x_cols):
        X_pred= predict_features[x_cols]
        pred_proba = clf.predict_proba(X_pred)
        pred = clf.predict(X_pred)
        logger.info(f'X_pred.shape={X_pred.shape}, pred_sum={np.sum(pred_proba[:, 1]>0.4)}')

        current_time = datetime.utcnow()
        current_timestamp = int(current_time.timestamp())
        res_df = pd.DataFrame({"billing_account_id": predict_features.billing_account_id.values.tolist(),
                                "is_target_pred": pred.astype(bool).tolist(),
                                "conf_is_target": pred_proba[:, 1].tolist(),
                                "conf_is_not_target": pred_proba[:, 0].tolist(),
                                "create_time": [current_timestamp]*predict_features.shape[0]})
        return res_df

    def save_predictions(self, clf, predict_features, x_cols):
        predictions: pd.DataFrame = self._prepare_prediction(clf, predict_features, x_cols)
        yt_schema = [
            {"name": "billing_account_id", "type": "string"},
            {"name": "is_target_pred", "type": "boolean"},
            {"name": "conf_is_target", "type": "double"},
            {"name": "conf_is_not_target", "type": "double"},
            {"name": "create_time", "type": "datetime"}
        ]
        self._yt_adapter.save_result(self._result_path, yt_schema, predictions, append=False)
