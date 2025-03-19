from sklearn.model_selection import StratifiedKFold, KFold, RepeatedStratifiedKFold
import numpy as np
from sklearn.model_selection import cross_validate
from consumption_predictor.experiment import Metrics
from clan_tools.utils.timing import timing


import numpy as np
from sklearn.base import BaseEstimator, ClassifierMixin
from sklearn.utils.validation import check_X_y, check_array, check_is_fitted
from sklearn.utils.multiclass import unique_labels
from sklearn.metrics import euclidean_distances
from sklearn.ensemble import RandomForestClassifier


class RFClassifier(RandomForestClassifier):

 
    def predict(self, X):
        pred = (self.predict_proba(X)[:,1] > 0.2).astype(int)
        return pred


@timing
def cross_val(clf, X, y, n_jobs=-1):
    scoring = {
               'prec': 'precision',
               'recall': 'recall',
              }
    kfold = StratifiedKFold(n_splits=5, shuffle=True, random_state=1)
    scores = cross_validate(clf, X, y, cv=kfold, n_jobs=n_jobs, scoring=scoring,  return_train_score=True)
    mean_rec, std_rec = np.mean(scores['test_recall']), np.std(scores['test_recall'])
    mean_prec, std_prec = np.mean(scores['test_prec']), np.std(scores['test_prec'])

    return Metrics(mean_rec, mean_prec, std_rec, std_prec)


