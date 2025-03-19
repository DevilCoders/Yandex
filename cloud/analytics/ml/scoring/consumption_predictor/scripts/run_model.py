# pylint: disable=no-value-for-parameter
import json
import logging.config
from dataclasses import asdict
from datetime import datetime
import click
import numpy as np
import pandas as pd
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_model.model.experiment import Experiment
from clan_tools.utils.timing import timing
from imblearn.over_sampling import ADASYN
from imblearn.pipeline import make_pipeline
from consumption_predictor.data_adapters.PredictionsAdapter import PredictionsAdapter
from consumption_predictor.model_selection import RFClassifier, cross_val
import os
from sklearn.preprocessing import StandardScaler
from consumption_predictor.feature_extraction.names_features import \
    add_names_features
from sklearn.feature_selection import SelectKBest
from sklearn.feature_selection import f_classif, VarianceThreshold
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def toXy(features):
    features.phone = features.phone.fillna(
        0).replace('', 0).astype(int).astype(str)
    features['phone_len'] = features.phone.str.len()
    features['phone_country7'] = (
        features.phone.str.get(0).astype(int) == 7).astype(int)
    features['phone_oper_code'] = pd.to_numeric(
        features.phone.str.slice(1, 4), errors='coerce').fillna(0)
    features['phone_oper_code_str'] = features.phone.str.slice(1, 4)
    features = add_names_features('account_name', features)

    x_cols = list(set(features.columns.values.tolist()) -
                  set(['unknown',  'first_use', 'start_week', 'end_week', 'url_hist',
                       'phone', 'email_domain', 'email', 'target', 'first_target_week',  'label', 'billing_account_id', 'result',  'account_name',
                       'phone_oper_code_str']))
    y_col = 'label'
    return features, x_cols, y_col


@click.command('run_model')
@click.option('--train_features_dir')
@click.option('--predict_features_dir')
@click.option('--predicts_dir')
@click.option('--model_name')
@timing
def run_model(train_features_dir, predict_features_dir, predicts_dir, model_name):
    yt_token = os.environ['YT_TOKEN']
    # configuration
    current_str_time = datetime.utcnow().isoformat(' ', 'seconds')
    # current_str_time = '2020-05-28 12:42:27'
    table_pred = f"{predicts_dir}/{current_str_time}"
    ch_adapter = ClickHouseYTAdapter(token=yt_token)

    yt_adapter = YTAdapter(yt_token)
    predictions_adapter = PredictionsAdapter(yt_adapter, table_pred)

    model_conf = dict(n_estimators=300, max_depth=25,
                      min_samples_leaf=2, random_state=0, n_jobs=-1)
    class_ratio = 0.1

    train_features_table = yt_adapter.last_table_name(train_features_dir)
    predict_features_table = yt_adapter.last_table_name(predict_features_dir)

    features = ch_adapter.execute_query(
        f'select * from "{train_features_table}"', to_pandas=True)
    predict_features = ch_adapter.execute_query(
        f'select * from "{predict_features_table}"', to_pandas=True)

    logger.debug(f'features count: {features.count()}')
    features, x_cols_train, y_col = toXy(features)
    predict_features, x_cols_predict, _ = toXy(predict_features)
    x_cols = list(set(x_cols_train).intersection(set(x_cols_predict)))
    logger.debug(x_cols)
    X = features[x_cols]
    y = features[y_col]
    class_ratio = np.mean(y)*1.5
    clf = RFClassifier(**model_conf)
    smote_clf = make_pipeline(VarianceThreshold(),
                              StandardScaler(),
                              SelectKBest(f_classif, k=50),
                              ADASYN(sampling_strategy=class_ratio,
                                     random_state=42),
                              clf)
    metrics = cross_val(smote_clf, X, y, n_jobs=1)
    smote_clf.fit(X, y)

    metrics.dataset_info = {"n_samples": len(y),
                            "n_targets": int(y.sum()),
                            "class_ratio": class_ratio,
                            # "yt_pred_path": table_pred
                            }

    final_features = (X.columns[
        smote_clf.named_steps['variancethreshold'].get_support()][
        smote_clf.named_steps['selectkbest'].get_support()])

    rf = smote_clf.named_steps['rfclassifier']
    feature_importances_df = (
        pd.DataFrame(rf.feature_importances_,
                     index=final_features,
                     columns=['importance'])
        .sort_values('importance',   ascending=False)
    )
    logger.debug(f'Feature importances: {feature_importances_df}')
    feature_importances = feature_importances_df.to_dict()
    metrics.feature_importances = feature_importances['importance']
    experiment = Experiment(model_name=f'{model_name} + {clf.__class__.__name__}',
                            model_conf=model_conf,
                            metrics=metrics)
    logger.info(experiment)

    predictions_adapter.save_predictions(smote_clf, predict_features, x_cols)

    with open('output.json', 'w') as f:
        json.dump(asdict(experiment), f)


if __name__ == '__main__':
    run_model()
