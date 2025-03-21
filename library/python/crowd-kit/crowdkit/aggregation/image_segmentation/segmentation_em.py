__all__ = ['SegmentationEM']

import attr
import numpy as np

from typing import List

from .. import annotations
from ..annotations import Annotation, manage_docstring
from ..base import BaseImageSegmentationAggregator


@attr.s
@manage_docstring
class SegmentationEM(BaseImageSegmentationAggregator):
    """
    The EM algorithm for the image segmentation task.

    This method performs a categorical aggregation task for each pixel: should
    it be included to the resulting aggregate or no. This task is solved by
    the single coin Dawid-Skene algorithm. Each worker has a latent parameter
    "skill" that shows the probability of this worker to answer correctly.
    Skills and true pixels' labels are optimized by the Expectation-Maximization
    algorithm.


    Doris Jung-Lin Lee. 2018.
    Quality Evaluation Methods for Crowdsourced Image Segmentation
    <https://ilpubs.stanford.edu:8090/1161/1/main.pdf>

    Args:
        n_iter: A number of EM iterations.

    Examples:
        >>> import numpy as np
        >>> import pandas as pd
        >>> from crowdkit.aggregation import SegmentationEM
        >>> df = pd.DataFrame(
        >>>     [
        >>>         ['t1', 'p1', np.array([[1, 0], [1, 1]])],
        >>>         ['t1', 'p2', np.array([[0, 1], [1, 1]])],
        >>>         ['t1', 'p3', np.array([[0, 1], [1, 1]])]
        >>>     ],
        >>>     columns=['task', 'worker', 'segmentation']
        >>> )
        >>> result = SegmentationEM().fit_predict(df)
    """

    n_iter: int = attr.ib(default=10)
    tol: float = attr.ib(default=1e-5)
    eps: float = 1e-15
    # segmentations_
    loss_history_: List[float] = attr.ib(init=False)

    @staticmethod
    @manage_docstring
    def _e_step(
        segmentations: annotations.SEGMENTATIONS,
        errors: annotations.SEGMENTATION_ERRORS,
        priors: annotations.IMAGE_PIXEL_PROBAS,
    ) -> annotations.IMAGE_PIXEL_PROBAS:
        """
        Perform E-step of algorithm.
        Given workers' segmentations and error vector and priors
        for each pixel calculates posteriori probabilities.
        """

        weighted_seg = np.multiply(errors, segmentations.T.astype(float)).T +\
                        np.multiply((1 - errors), (1 - segmentations).T.astype(float)).T

        with np.errstate(divide='ignore'):
            pos_log_prob = np.log(priors) + np.log(weighted_seg).sum(axis=0)
            neg_log_prob = np.log(1 - priors) + np.log(1 - weighted_seg).sum(axis=0)

            with np.errstate(invalid='ignore'):
                # division by the denominator in the Bayes formula
                posteriors = np.nan_to_num(np.exp(pos_log_prob) / (np.exp(pos_log_prob) + np.exp(neg_log_prob)), nan=0)

        return posteriors

    @staticmethod
    @manage_docstring
    def _m_step(
        segmentations: annotations.SEGMENTATIONS,
        posteriors: annotations.IMAGE_PIXEL_PROBAS,
        segmentation_region_size: int,
        segmentations_sizes: np.ndarray
    ) -> annotations.SEGMENTATION_ERRORS:
        """
        Perform M-step of algorithm.
        Given a priori probabilities for each pixel and the segmentation of the workers,
        it estimates worker's errors probabilities vector.
        """

        mean_errors_expectation = (segmentations_sizes + posteriors.sum() -
                                   2 * (segmentations * posteriors).sum(axis=(1, 2))) / segmentation_region_size

        # return probability of worker marking pixel correctly
        return 1 - mean_errors_expectation

    def _evidence_lower_bound(
        self,
        segmentations: annotations.SEGMENTATIONS,
        priors: annotations.IMAGE_PIXEL_PROBAS,
        posteriors: annotations.IMAGE_PIXEL_PROBAS,
        errors: annotations.SEGMENTATION_ERRORS
    ):
        weighted_seg = (np.multiply(errors, segmentations.T.astype(float)).T +
                        np.multiply((1 - errors), (1 - segmentations).T.astype(float)).T)

        # we handle log(0) * 0 == 0 case with nan_to_num so warnings are irrelevant here
        with np.errstate(divide='ignore', invalid='ignore'):
            log_likelihood_expectation = \
                np.nan_to_num((np.log(weighted_seg) + np.log(priors)[None, ...]) * posteriors, nan=0).sum() +\
                np.nan_to_num((np.log(1 - weighted_seg) + np.log(1 - priors)[None, ...]) * (1 - posteriors), nan=0).sum()

            return log_likelihood_expectation - np.nan_to_num(np.log(posteriors) * posteriors, nan=0).sum()

    @manage_docstring
    def _aggregate_one(self, segmentations: annotations.SEGMENTATIONS) -> annotations.SEGMENTATION:
        """
        Performs an expectation maximization algorithm for a single image.
        """
        priors = sum(segmentations) / len(segmentations)
        segmentations = np.stack(segmentations.values)
        segmentation_region_size = segmentations.any(axis=0).sum()
        if segmentation_region_size == 0:
            return np.zeros_like(segmentations[0])

        segmentations_sizes = segmentations.sum(axis=(1, 2))
        # initialize with errors assuming that ground truth segmentation is majority vote
        errors = self._m_step(segmentations, np.round(priors), segmentation_region_size, segmentations_sizes)
        loss = -np.inf
        self.loss_history_ = []
        for _ in range(self.n_iter):
            posteriors = self._e_step(segmentations, errors, priors)
            posteriors[posteriors < self.eps] = 0
            errors = self._m_step(segmentations, posteriors, segmentation_region_size, segmentations_sizes)
            new_loss = self._evidence_lower_bound(segmentations, priors, posteriors, errors) / (len(segmentations) * segmentations[0].size)
            priors = posteriors
            self.loss_history_.append(new_loss)
            if new_loss - loss < self.tol:
                break
            loss = new_loss

        return priors > 0.5

    @manage_docstring
    def fit(self, data: annotations.SEGMENTATION_DATA) -> Annotation(type='SegmentationEM', title='self'):
        """
        Fit the model.
        """

        data = data[['task', 'worker', 'segmentation']]

        self.segmentations_ = data.groupby('task').segmentation.apply(
            lambda segmentations: self._aggregate_one(segmentations)  # using lambda for python 3.7 compatibility
        )
        return self

    @manage_docstring
    def fit_predict(self, data: annotations.SEGMENTATION_DATA) -> annotations.TASKS_SEGMENTATIONS:
        """
        Fit the model and return the aggregated segmentations.
        """
        return self.fit(data).segmentations_
