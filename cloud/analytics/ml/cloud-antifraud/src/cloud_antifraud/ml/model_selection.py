from sklearn.model_selection import StratifiedKFold, KFold, RepeatedStratifiedKFold
import numpy as np
from sklearn.model_selection import cross_validate
from clan_tools.utils.timing import timing
from dataclasses import dataclass


@dataclass
class Metrics:
    mean_rec: float
    mean_prec: float
    std_rec: float
    std_prec: float


@timing
def cross_val(clf, X, y):
    scoring = {
        'prec': 'precision',
        'recall': 'recall',
    }
    kfold = StratifiedKFold(n_splits=10, shuffle=True, random_state=1)
    scores = cross_validate(clf, X, y, cv=kfold, n_jobs=10,
                            scoring=scoring,  return_train_score=True)
    mean_rec, std_rec = np.mean(
        scores['test_recall']), np.std(scores['test_recall'])
    mean_prec, std_prec = np.mean(
        scores['test_prec']), np.std(scores['test_prec'])

    return Metrics(mean_rec, mean_prec, std_rec, std_prec)

