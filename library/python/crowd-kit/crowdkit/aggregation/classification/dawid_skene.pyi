__all__ = [
    'DawidSkene',
    'OneCoinDawidSkene',
]
import crowdkit.aggregation.base
import pandas
import typing


class DawidSkene(crowdkit.aggregation.base.BaseClassificationAggregator):
    """Dawid-Skene aggregation model.


    Probabilistic model that parametrizes workers' level of expertise through confusion matrices.

    Let $e^w$ be a worker's confusion (error) matrix of size $K \times K$ in case of $K$ class classification,
    $p$ be a vector of prior classes probabilities, $z_j$ be a true task's label, and $y^w_j$ be a worker's
    answer for the task $j$. The relationships between these parameters are represented by the following latent
    label model.

    ![Dawid-Skene latent label model](https://tlk.s3.yandex.net/crowd-kit/docs/ds_llm.png)

    Here the prior true label probability is
    $$
    \operatorname{Pr}(z_j = c) = p[c],
    $$
    and the distribution on the worker's responses given the true label $c$ is represented by the
    corresponding column of the error matrix:
    $$
    \operatorname{Pr}(y_j^w = k | z_j = c) = e^w[k, c].
    $$

    Parameters $p$ and $e^w$ and latent variables $z$ are optimized through the Expectation-Maximization algorithm.

    A. Philip Dawid and Allan M. Skene. Maximum Likelihood Estimation of Observer Error-Rates Using the EM Algorithm.
    *Journal of the Royal Statistical Society. Series C (Applied Statistics), Vol. 28*, 1 (1979), 20–28.

    <https://doi.org/10.2307/2346806>

    Args:
        n_iter: The number of EM iterations.

    Examples:
        >>> from crowdkit.aggregation import DawidSkene
        >>> from crowdkit.datasets import load_dataset
        >>> df, gt = load_dataset('relevance-2')
        >>> ds = DawidSkene(100)
        >>> result = ds.fit_predict(df)
    Attributes:
        labels_ (typing.Optional[pandas.core.series.Series]): Tasks' labels.
            A pandas.Series indexed by `task` such that `labels.loc[task]`
            is the tasks's most likely true label.

        probas_ (typing.Optional[pandas.core.frame.DataFrame]): Tasks' label probability distributions.
            A pandas.DataFrame indexed by `task` such that `result.loc[task, label]`
            is the probability of `task`'s true label to be equal to `label`. Each
            probability is between 0 and 1, all task's probabilities should sum up to 1

        priors_ (typing.Optional[pandas.core.series.Series]): A prior label distribution.
            A pandas.Series indexed by labels and holding corresponding label's
            probability of occurrence. Each probability is between 0 and 1,
            all probabilities should sum up to 1

        errors_ (typing.Optional[pandas.core.frame.DataFrame]): Workers' error matrices.
            A pandas.DataFrame indexed by `worker` and `label` with a column for every
            label_id found in `data` such that `result.loc[worker, observed_label, true_label]`
            is the probability of `worker` producing an `observed_label` given that a task's
            true label is `true_label`
    """

    def fit(self, data: pandas.DataFrame) -> 'DawidSkene':
        """Fit the model through the EM-algorithm.
        Args:
            data (DataFrame): Workers' labeling results.
                A pandas.DataFrame containing `task`, `worker` and `label` columns.
        Returns:
            DawidSkene: self.
        """
        ...

    def fit_predict_proba(self, data: pandas.DataFrame) -> pandas.DataFrame:
        """Fit the model and return probability distributions on labels for each task.
        Args:
            data (DataFrame): Workers' labeling results.
                A pandas.DataFrame containing `task`, `worker` and `label` columns.
        Returns:
            DataFrame: Tasks' label probability distributions.
                A pandas.DataFrame indexed by `task` such that `result.loc[task, label]`
                is the probability of `task`'s true label to be equal to `label`. Each
                probability is between 0 and 1, all task's probabilities should sum up to 1
        """
        ...

    def fit_predict(self, data: pandas.DataFrame) -> pandas.Series:
        """Fit the model and return aggregated results.
        Args:
            data (DataFrame): Workers' labeling results.
                A pandas.DataFrame containing `task`, `worker` and `label` columns.
        Returns:
            Series: Tasks' labels.
                A pandas.Series indexed by `task` such that `labels.loc[task]`
                is the tasks's most likely true label.
        """
        ...

    def __init__(
        self,
        n_iter: int = 100,
        tol: float = 1e-05
    ) -> None:
        """Method generated by attrs for class DawidSkene.
        """
        ...

    labels_: typing.Optional[pandas.Series]
    n_iter: int
    tol: float
    probas_: typing.Optional[pandas.DataFrame]
    priors_: typing.Optional[pandas.Series]
    errors_: typing.Optional[pandas.DataFrame]
    loss_history_: typing.List[float]


class OneCoinDawidSkene(DawidSkene):
    """One-coin Dawid-Skene aggregation model.

    This model works exactly like original Dawid-Skene model based on EM Algorithm except for workers' error calculation
    on M-step of the algorithm.

    First the workers' skills are calculated as their accuracy in accordance with labels probability.
    Let $e^w$ be a worker's confusion (error) matrix of size $K \times K$ in case of $K$ class classification,
    $p$ be a vector of prior classes probabilities, $z_j$ be a true task's label, and $y^w_j$ be a worker's
    answer for the task $j$. Let s_{w} be a worker's skill (accuracy). Then the error
    $$
    e^w_{j,z_j}  = \begin{cases}
        s_{w} & y^w_j = z_j \\
        \frac{1 - s_{w}}{K - 1} & y^w_j \neq z_j
    \end{cases}
    $$

    Args:
        n_iter: The number of EM iterations.

    Examples:
        >>> from crowdkit.aggregation import OneCoinDawidSkene
        >>> from crowdkit.datasets import load_dataset
        >>> df, gt = load_dataset('relevance-2')
        >>> hds = OneCoinDawidSkene(100)
        >>> result = hds.fit_predict(df)
    Attributes:
        labels_ (typing.Optional[pandas.core.series.Series]): Tasks' labels.
            A pandas.Series indexed by `task` such that `labels.loc[task]`
            is the tasks's most likely true label.

        probas_ (typing.Optional[pandas.core.frame.DataFrame]): Tasks' label probability distributions.
            A pandas.DataFrame indexed by `task` such that `result.loc[task, label]`
            is the probability of `task`'s true label to be equal to `label`. Each
            probability is between 0 and 1, all task's probabilities should sum up to 1

        priors_ (typing.Optional[pandas.core.series.Series]): A prior label distribution.
            A pandas.Series indexed by labels and holding corresponding label's
            probability of occurrence. Each probability is between 0 and 1,
            all probabilities should sum up to 1

        errors_ (typing.Optional[pandas.core.frame.DataFrame]): Workers' error matrices.
            A pandas.DataFrame indexed by `worker` and `label` with a column for every
            label_id found in `data` such that `result.loc[worker, observed_label, true_label]`
            is the probability of `worker` producing an `observed_label` given that a task's
            true label is `true_label`
    """

    def __init__(
        self,
        n_iter: int = 100,
        tol: float = 1e-05
    ) -> None:
        """Method generated by attrs for class OneCoinDawidSkene.
        """
        ...

    labels_: typing.Optional[pandas.Series]
    n_iter: int
    tol: float
    probas_: typing.Optional[pandas.DataFrame]
    priors_: typing.Optional[pandas.Series]
    errors_: typing.Optional[pandas.DataFrame]
    loss_history_: typing.List[float]
