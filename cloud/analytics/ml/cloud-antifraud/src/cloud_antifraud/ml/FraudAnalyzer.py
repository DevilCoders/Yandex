from clan_tools.utils.cache import cached
from clan_tools.utils.timing import timing
from cloud_antifraud.ml.MLProblem import MLProblem
from cloud_antifraud.ml.model_selection import cross_val
from sklearn.ensemble import RandomForestClassifier, ExtraTreesClassifier
from imblearn.over_sampling import ADASYN, SMOTE, RandomOverSampler, SMOTENC
from imblearn.pipeline import make_pipeline
from sklearn.svm import SVC
from sklearn.neighbors import KNeighborsClassifier
from catboost import CatBoostClassifier
import logging
import umap
import numpy as np
import lime 
import lime.lime_tabular
import pandas as pd
import shap

logger = logging.getLogger(__name__)
from sklearn.ensemble import RandomTreesEmbedding

class FraudAnalyzer:
    def __init__(self, problem:MLProblem):
        self._problem = problem

    @staticmethod
    def _refine_types(features_dash):
        bool_cols = features_dash.columns[features_dash.dtypes == bool].values
        features_dash[bool_cols] = features_dash[bool_cols].astype(int)
        obj_cols = features_dash.columns[features_dash.dtypes == object].values
        features_dash[obj_cols] = features_dash[obj_cols].astype(str)
        float32_cols = features_dash.columns[features_dash.dtypes == np.float32].values
        features_dash[float32_cols] = features_dash[float32_cols].astype(float)
        return features_dash

    
    @staticmethod
    # @cached('data/cache/umap.pkl')
    @timing
    def _umap(data, pred):
        # y = pred[:, 1].copy()
        # y[(y > 0.3) & (y < 0.7)] = -1
        return umap.UMAP(metric='hamming').fit_transform(data)
    
 
        
    @timing
    # @cached('data/cache/analyze.pkl')
    def analyze(self):
        X, y, features = self._problem.X, self._problem.y, self._problem.features
        
        model_conf = dict(n_estimators=500, max_depth=25, min_samples_leaf=5, 
                          max_features=0.70, bootstrap=True, class_weight='balanced', random_state=0)
        class_ratio=0.5
        clf = ExtraTreesClassifier(**model_conf)
        # clf = make_pipeline(SMOTENC( random_state=42, categorical_features=cb_categories), clf)

        antifraud_recreation_metrics = cross_val(clf, X, features['is_target'].astype(int) )
        logger.info(f'antifraud_recreation_metrics = {antifraud_recreation_metrics}')
        
        X_train = X.iloc[(y!=-1).values,:]
        y_train = y[y!=-1]

        antifraud_train_metrics = cross_val(clf, X_train, features['is_target'].astype(int)[y!=-1] )
        logger.info(f'antifraud_train_metrics = {antifraud_train_metrics}')
        
        clf.fit(X_train, y_train)
        leaves = clf.apply(X)
        trees_pred:np.ndarray = clf.predict_proba(X)        
        u = self._umap(leaves, trees_pred)
        
        features_dash = features.copy()
        features_dash['u0'] = u[:, 0]
        features_dash['u1'] = u[:, 1]
        features_dash['y'] = y.values
        features_dash['y_back'] = y.values
        features_dash['conf0'] = trees_pred[:, 0]
        features_dash['conf1'] = trees_pred[:, 1]
        features_dash = self._refine_types(features_dash)

            
        explainer = lime.lime_tabular.LimeTabularExplainer(features_dash[X.columns.values].iloc[(y!=-1).values,:].values, 
                                            class_names=['ok', 'fraud'], 
                                            feature_names = X.columns.values,
                                            categorical_features=self._problem.description.cat_features_idx, 
                                            categorical_names=self._problem.description.cat_features,
                                            # kernel_width=3,
                                             verbose=True)

        logger.debug("Calculating SHAP values")
        shap_explainer = shap.TreeExplainer(clf)

        
        return features_dash, clf, explainer, shap_explainer