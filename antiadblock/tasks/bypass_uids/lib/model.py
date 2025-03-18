import os

import pandas as pd
from sklearn.model_selection import train_test_split
from catboost import CatBoostClassifier, Pool

from antiadblock.tasks.tools.logger import create_logger
import antiadblock.tasks.bypass_uids.lib.data as data
from antiadblock.tasks.bypass_uids.lib.util import upload_file_to_sandbox, load_file_from_sandbox

logger = create_logger(__file__)

PROCN = 20
MAX_TRAIN_ITERATIONS = 1000

THRESHOLD = 0.4
MIN_ACCURACY = 90
MIN_RECALL = 50

FILENAME = os.path.abspath('model.dump')
LOADED_FILENAME = os.path.abspath('loaded_model.dump')

RESOURCE_TYPE = 'ANTIADBLOCK_BYPASS_UIDS_MODEL'


class ModelWrapper:
    def __init__(self):
        self.df = None
        self.cat_features_ind = None
        self.X_train, self.X_validation, self.y_train, self.y_validation = (None,) * 4
        self.model = CatBoostClassifier(
            iterations=MAX_TRAIN_ITERATIONS,
            learning_rate=0.1,
            eval_metric='Accuracy',
            random_seed=42,
            logging_level='Verbose',
            # early stopping params to avoid overfitting
            od_type='Iter',
            od_wait=40,
            thread_count=PROCN,
        )

    def train(self, train_df):
        logger.info('preparing data')
        self.df = train_df
        X = self.df[data.CAT_FEATURES + data.FLOAT_FEATURES]
        self.cat_features_ind = list(range(len(data.CAT_FEATURES)))
        y = self.df[data.TARGET_FEATURES]
        self.X_train, self.X_validation, self.y_train, self.y_validation = train_test_split(X, y, train_size=0.8, random_state=42)
        for df in (self.X_train, self.X_validation, self.y_train, self.y_validation):
            logger.info(df.head(10))
        logger.info('data prepared, starting to fit model')
        self.model.fit(
            self.X_train, self.y_train,
            cat_features=self.cat_features_ind,
            eval_set=(self.X_validation, self.y_validation),
            verbose=True,
        )
        logger.info('model trained')
        feature_importances = self.model.get_feature_importance(Pool(self.X_validation, self.y_validation, cat_features=self.cat_features_ind))
        feature_names = self.X_validation.columns
        logger.info('features importance:')
        for score, name in sorted(zip(feature_importances, feature_names), reverse=True):
            logger.info(f'{name}{" " * (30 - len(name))}{score}')

    def evaluate_accuracy(self, test_df=None, threshold=THRESHOLD, device=data.Device.all.name):
        """
        calculate some production metrics here, same as in yql data.evaluate_model_accuracy
        :return: dict with accuracy results
        """
        if test_df is None:
            X, y = self.X_validation, self.y_validation
        else:
            X = test_df[data.CAT_FEATURES + data.FLOAT_FEATURES]
            y = test_df[data.TARGET_FEATURES]
        if device != 'all':
            device_index = X[data.Columns.device.name] == device
            X = X[device_index]
            y = y[device_index]

        df = pd.DataFrame()
        df.loc[device, 'total_rows'] = len(y)

        positive_index = y.iloc[:, 0] == 1
        df.loc[device, 'positive_rows'] = positive_index.sum()

        predictions = self.model.predict(X, prediction_type='Probability')
        predictions = predictions[:, 1] >= threshold

        df.loc[device, 'TP'] = predictions[positive_index].sum()
        df.loc[device, 'FN'] = df.loc[device, 'positive_rows'] - df.loc[device, 'TP']  # Adblock predicted as noAdblock
        df.loc[device, 'FP'] = predictions[~positive_index].sum()  # noAdblock predicted as Adblock
        df.loc[device, 'TN'] = df.loc[device, 'total_rows'] - df.loc[device, 'positive_rows'] - df.loc[device, 'FP']

        df.loc[device, 'recall'] = 100. * df.loc[device, 'TP'] / (df.loc[device, 'TP'] + df.loc[device, 'FN'])
        df.loc[device, 'accuracy'] = 100. * (df.loc[device, 'TP'] + df.loc[device, 'TN']) / df.loc[device, 'total_rows']
        return df

    def load(self, resource_id=None):
        if resource_id:
            load_file_from_sandbox(resource_id, LOADED_FILENAME)
            self.model.load_model(LOADED_FILENAME)
        else:
            self.model.load_model(FILENAME)

    def save(self, validated=False):
        description = 'catboost model, predicting if uniqid have Adblock'
        self.model.save_model(FILENAME)
        if validated:
            resource_id = upload_file_to_sandbox(FILENAME, resource_type=RESOURCE_TYPE, description=description, ttl='inf')
        else:
            resource_id = upload_file_to_sandbox(FILENAME, description=description)
        return resource_id
