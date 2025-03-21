__all__ = ['MMSR']

from typing import Optional, Tuple, List

import attr
import numpy as np
import pandas as pd
import scipy.sparse.linalg as sla
import scipy.stats as sps

from .. import annotations
from ..annotations import manage_docstring, Annotation
from ..base import BaseClassificationAggregator
from ..utils import named_series_attrib
from .majority_vote import MajorityVote


@attr.s
@manage_docstring
class MMSR(BaseClassificationAggregator):
    r"""
    Matrix Mean-Subsequence-Reduced Algorithm.

    The M-MSR assumes that workers have different level of expertise and associated
    with a vector of "skills" $\boldsymbol{s}$ which entries $s_i$ show the probability
    of the worker $i$ to answer correctly to the given task. Having that, we can show that
    $$
    \mathbb{E}\left[\frac{M}{M-1}\widetilde{C}-\frac{1}{M-1}\boldsymbol{1}\boldsymbol{1}^T\right]
     = \boldsymbol{s}\boldsymbol{s}^T,
    $$
    where $M$ is the total number of classes, $\widetilde{C}$ is a covariation matrix between
    workers, and $\boldsymbol{1}\boldsymbol{1}^T$ is the all-ones matrix which has the same
    size as $\widetilde{C}$.


    So, the problem of recovering the skills vector $\boldsymbol{s}$ becomes equivalent to the
    rank-one matrix completion problem. The M-MSR algorithm is an iterative algorithm for *rubust*
    rank-one matrix completion, so its result is an estimator of the vector $\boldsymbol{s}$.
    Then, the aggregation is the weighted majority vote with weights equal to
    $\log \frac{(M-1)s_i}{1-s_i}$.

    Matrix Mean-Subsequence-Reduced Algorithm. Qianqian Ma and Alex Olshevsky.
    Adversarial Crowdsourcing Through Robust Rank-One Matrix Completion.
    *34th Conference on Neural Information Processing Systems (NeurIPS 2020)*

    <https://arxiv.org/abs/2010.12181>

    Args:
        n_iter: The maximum number of iterations of the M-MSR algorithm.
        eps: Convergence threshold.
        random_state: Seed number for the random initialization.

    Examples:
        >>> from crowdkit.aggregation import MMSR
        >>> from crowdkit.datasets import load_dataset
        >>> df, gt = load_dataset('relevance-2')
        >>> mmsr = MMSR()
        >>> result = mmsr.fit_predict(df)
    """
    n_iter: int = attr.ib(default=10000)
    tol: float = attr.ib(default=1e-10)
    random_state: Optional[int] = attr.ib(default=0)
    _observation_matrix: np.ndarray = attr.ib(factory=lambda: np.array([]))
    _covariation_matrix: np.ndarray = attr.ib(factory=lambda: np.array([]))
    _n_common_tasks: np.ndarray = attr.ib(factory=lambda: np.array([]))
    _n_workers: int = attr.ib(default=0)
    _n_tasks: int = attr.ib(default=0)
    _n_labels: int = attr.ib(default=0)
    _labels_mapping: dict = attr.ib(factory=dict)
    _workers_mapping: dict = attr.ib(factory=dict)
    _tasks_mapping: dict = attr.ib(factory=dict)

    # Available after fit
    skills_: annotations.OPTIONAL_SKILLS = named_series_attrib(name='skill')

    # Available after predict or predict_score
    # labels_
    scores_: annotations.OPTIONAL_SCORES = attr.ib(init=False)

    loss_history_: List[float] = attr.ib(init=False)

    @manage_docstring
    def _apply(self, data: annotations.LABELED_DATA) -> Annotation(type='MMSR', title='self'):
        mv = MajorityVote().fit(data, skills=self.skills_)
        self.labels_ = mv.labels_
        self.scores_ = mv.probas_
        return self

    @manage_docstring
    def fit(self, data: annotations.LABELED_DATA) -> Annotation(type='MMSR', title='self'):
        """
        Estimate the workers' skills.
        """

        data = data[['task', 'worker', 'label']]
        self._construnct_covariation_matrix(data)
        self._m_msr()
        return self

    @manage_docstring
    def predict(self, data: annotations.LABELED_DATA) -> annotations.TASKS_LABELS:
        """
        Infer the true labels when the model is fitted.
        """

        return self._apply(data).labels_

    @manage_docstring
    def predict_score(self, data: annotations.LABELED_DATA) -> annotations.TASKS_LABEL_SCORES:
        """
        Return total sum of weights for each label when the model is fitted.
        """

        return self._apply(data).scores_

    @manage_docstring
    def fit_predict(self, data: annotations.LABELED_DATA) -> annotations.TASKS_LABELS:
        """
        Fit the model and return aggregated results.
        """

        return self.fit(data).predict(data)

    @manage_docstring
    def fit_predict_score(self, data: annotations.LABELED_DATA) -> annotations.TASKS_LABEL_SCORES:
        """
        Fit the model and return the total sum of weights for each label.
        """

        return self.fit(data).predict_score(data)

    def _m_msr(self) -> None:
        F_param = int(np.floor(self._sparsity / 2)) - 1
        n, m = self._covariation_matrix.shape
        u = sps.uniform.rvs(size=(n, 1), random_state=self.random_state)
        v = sps.uniform.rvs(size=(m, 1), random_state=self.random_state)
        observed_entries = np.abs(np.sign(self._n_common_tasks)) == 1
        X = np.abs(self._covariation_matrix)
        self.loss_history_ = []
        for _ in range(self.n_iter):
            v_prev = np.copy(v)
            u_prev = np.copy(u)
            for j in range(n):
                target_v = X[:, j].reshape(-1, 1)
                target_v = target_v[observed_entries[:, j]] / u[observed_entries[:, j]]

                y = self._remove_largest_and_smallest_F_value(target_v, F_param, v[j][0], self._n_tasks)
                if len(y) == 0:
                    v[j] = v[j]
                else:
                    v[j][0] = y.mean()

            for i in range(m):
                target_u = X[i, :].reshape(-1, 1)
                target_u = target_u[observed_entries[i, :]] / v[observed_entries[i, :]]
                y = self._remove_largest_and_smallest_F_value(target_u, F_param, u[i][0], self._n_tasks)
                if len(y) == 0:
                    u[i] = u[i]
                else:
                    u[i][0] = y.mean()

            loss = np.linalg.norm(u @ v.T - u_prev @ v_prev.T, ord='fro')
            self.loss_history_.append(loss)
            if loss < self.tol:
                break

        k = np.sqrt(np.linalg.norm(u) / np.linalg.norm(v))
        x_track_1 = u / k
        x_track_2 = self._sign_determination_valid(self._covariation_matrix, x_track_1)
        x_track_3 = np.minimum(x_track_2, 1 - 1. / np.sqrt(self._n_tasks))
        x_MSR = np.maximum(x_track_3, -1 / (self._n_labels - 1) + 1. / np.sqrt(self._n_tasks))

        workers_probas = x_MSR * (self._n_labels - 1) / (self._n_labels) + 1 / self._n_labels
        workers_probas = workers_probas.ravel()
        skills = np.log(workers_probas * (self._n_labels - 1) / (1 - workers_probas))

        self.skills_ = self._get_skills_from_array(skills)

    @manage_docstring
    def _get_skills_from_array(self, array: np.ndarray) -> annotations.SKILLS:
        inverse_workers_mapping = {ind: worker for worker, ind in self._workers_mapping.items()}
        index = [inverse_workers_mapping[i] for i in range(len(array))]
        return pd.Series(array, index=pd.Index(index, name='worker'))

    @staticmethod
    def _sign_determination_valid(C: np.ndarray, s_abs: np.ndarray) -> np.ndarray:
        S = np.sign(C)
        n = len(s_abs)

        valid_idx = np.where(np.sum(C, axis=1) != 0)[0]
        S_valid = S[valid_idx[:, None], valid_idx]
        k = S_valid.shape[0]
        upper_idx = np.triu(np.ones(shape=(k, k)))
        S_valid_upper = S_valid * upper_idx
        new_node_end_I, new_node_end_J = np.where(S_valid_upper == 1)
        S_valid[S_valid == 1] = 0
        I = np.eye(k)
        S_valid_new = I[new_node_end_I, :] + I[new_node_end_J, :]
        m = S_valid_new.shape[0]
        A = np.vstack((np.hstack((np.abs(S_valid), S_valid_new.T)), np.hstack((S_valid_new, np.zeros(shape=(m, m))))))
        n_new = A.shape[0]
        W = (1. / np.sum(A, axis=1)).reshape(-1, 1) @ np.ones(shape=(1, n_new)) * A
        D, V = sla.eigs(W + np.eye(n_new), 1, which='SM')
        V = V.real
        sign_vector = np.sign(V)
        s_sign = np.zeros(shape=(n, 1))
        s_sign[valid_idx] = np.sign(np.sum(sign_vector[:k])) * s_abs[valid_idx] * sign_vector[:k]
        return s_sign

    @staticmethod
    def _remove_largest_and_smallest_F_value(x, F, a, n_tasks) -> np.ndarray:
        y = np.sort(x, axis=0)
        if np.sum(y < a) < F:
            y = y[y[:, 0] >= a]
        else:
            y = y[F:]

        m = y.shape[0]
        if np.sum(y > a) < F:
            y = y[y[:, 0] <= a]
        else:
            y = np.concatenate((y[:m - F], y[m:]), axis=0)
        if len(y) == 1 and y[0][0] == 0:
            y[0][0] = 1 / np.sqrt(n_tasks)
        return y

    @manage_docstring
    def _construnct_covariation_matrix(self, answers: annotations.LABELED_DATA) -> Tuple[np.ndarray]:
        labels = pd.unique(answers.label)
        self._n_labels = len(labels)
        self._labels_mapping = {labels[idx]: idx + 1 for idx in range(self._n_labels)}

        workers = pd.unique(answers.worker)
        self._n_workers = len(workers)
        self._workers_mapping = {workers[idx]: idx for idx in range(self._n_workers)}

        tasks = pd.unique(answers.task)
        self._n_tasks = len(tasks)
        self._tasks_mapping = {tasks[idx]: idx for idx in range(self._n_tasks)}

        self._observation_matrix = np.zeros(shape=(self._n_workers, self._n_tasks))
        for i, row in answers.iterrows():
            self._observation_matrix[self._workers_mapping[row['worker']]][self._tasks_mapping[row['task']]] = \
            self._labels_mapping[row['label']]

        self._n_common_tasks = np.sign(self._observation_matrix) @ np.sign(self._observation_matrix).T
        self._n_common_tasks -= np.diag(np.diag(self._n_common_tasks))
        self._sparsity = np.min(np.sign(self._n_common_tasks).sum(axis=0))

        # Can we rewrite it in matrix operations?
        self._covariation_matrix = np.zeros(shape=(self._n_workers, self._n_workers))
        for i in range(self._n_workers):
            for j in range(self._n_workers):
                if self._n_common_tasks[i][j]:
                    valid_idx = np.sign(self._observation_matrix[i]) * np.sign(self._observation_matrix[j])
                    self._covariation_matrix[i][j] = np.sum((self._observation_matrix[i] == self._observation_matrix[j]) * valid_idx) / self._n_common_tasks[i][j]

        self._covariation_matrix *= self._n_labels / (self._n_labels - 1)
        self._covariation_matrix -= np.ones(shape=(self._n_workers, self._n_workers)) / (self._n_labels - 1)
