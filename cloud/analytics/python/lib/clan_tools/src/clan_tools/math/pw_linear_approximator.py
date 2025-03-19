# type: ignore

import numpy as np
from numba import njit
from typing import Tuple, List


class PiecewiseLinearFunction:
    def __init__(self, base_points: np.ndarray):
        self.base_points = [(point[0], point[1]) for point in base_points]
        self.xx = base_points[:, 0]
        self.yy = base_points[:, 1]
        self.ii = base_points[:, 2]
        self._alphas = None

    @property
    def alphas(self):
        if self._alphas is None:
            self._alphas = self._get_alphas()
        return self._alphas

    def _get_alphas(self) -> List[float]:
        alphas = []
        for point1, point2 in zip(self.base_points[:-1], self.base_points[1:]):
            alphas.append(get_alpha(point1, point2))
        return alphas

    def is_rising(self, alpha_threshold):
        is_rising = None

        alphas = []
        for ind, point in enumerate(self.base_points):
            if ind != (len(self.base_points) - 1):
                alpha = get_alpha(np.array(point), np.array(self.base_points[ind + 1]))
                alphas.append(alpha)

                if abs(alpha) > alpha_threshold:
                    is_rising = 1 if alpha > 0 else 0
                    break
        if is_rising is None:
            is_rising = 1 if alphas[0] > 0 else 0
        return is_rising

    def get_front_ids(self, alpha_threshold: float, severity_threshold: float = 0.3, front_ratio: float = 1.0) -> \
            Tuple[int, int, int]:
        alpha_flag, severity_flag, sign_flag = False, False, False
        prefront_part, front_part, front_severity = 0, 0, 0
        start_front_idx, end_front_ind = 0, 0
        prestart_front, start_front, end_front = None, None, None

        for idx, point in enumerate(self.base_points[:-2]):
            if idx != (len(self.base_points) - 2):
                point1 = self.base_points[idx + 1]
                point2 = self.base_points[idx + 2]

                alpha = get_alpha(point, point1)
                next_alpha = get_alpha(point1, point2)
                sign = 1 if alpha > 0 else -1
                next_sign = 1 if next_alpha > 0 else -1

                # alpha flag
                if idx != 0 and abs(alpha) > alpha_threshold:
                    alpha_flag = True
                start_front_idx = idx + 1 if not alpha_flag else start_front_idx

                #  sign flag
                if alpha_flag and not sign_flag:
                    if next_sign != sign or (next_sign == sign and abs(next_alpha) < alpha_threshold / 3):
                        sign_flag = True

                # front and prefront parts
                if not alpha_flag:
                    prefront_part += point1[0] - point[0]
                    start_front_idx = idx + 1
                else:
                    front_part += point1[0] - point[0]
                    cur_points = [point[1] for point in self.base_points][start_front_idx:idx + 2]
                    front_severity = max(cur_points) - min(cur_points)

                # severity flag
                if front_severity > severity_threshold:
                    severity_flag = True

                if alpha_flag and severity_flag and sign_flag:
                    end_front_ind = idx + 1
                    prestart_front = self.ii[0]
                    start_front = self.ii[start_front_idx]
                    end_front = self.ii[end_front_ind]

                    if prefront_part > front_ratio * front_part:
                        prestart_front = (1 + front_ratio) * self.ii[start_front_idx] - front_ratio * end_front
                        prestart_front = prestart_front if prestart_front > 0 else self.ii[0]

                    break

        if prestart_front is None or start_front is None or end_front is None:
            prestart_front = self.ii[0]
            end_front = self.ii[-1]
            start_front = (prestart_front + end_front) // 2

        return prestart_front, start_front, end_front


class PiecewiseLinearApproximator:
    def __init__(self, dist_threshold: float = 0.1, window_size: int = 200, skip_size: int = 50):
        self._dist_threshold = dist_threshold
        self._window_size = window_size
        self._skip_size = skip_size

    def approximate(self, y: np.ndarray) -> Tuple[PiecewiseLinearFunction, Tuple[np.ndarray, np.ndarray]]:
        """
        Approximate y via pwf based on points_pwf
        """
        yy = y[~np.isnan(y)]
        xx = np.linspace(0, len(yy) / self._window_size, len(yy))
        ts_line = np.vstack((xx, yy)).T
        points_pwf = np.array([[xx[0], yy[0], 0], [xx[-1], yy[-1], len(xx) - 1]])

        max_dist = np.inf
        while max_dist > self._dist_threshold:
            max_dist, points_pwf = add_farthest_point(ts_line, points_pwf, self._skip_size)

        pwf = PiecewiseLinearFunction(points_pwf)
        return pwf, (xx, yy)


# @njit
def add_farthest_point(ts_line: np.ndarray, points: np.ndarray, skip_size: float) -> Tuple[float, np.ndarray]:
    """
    Finds the point on the ts_line that is the farthest from pwf based on points. Adds this point to points
    """
    pwf_line = np.vstack((ts_line[:, 0], get_pwf_line(ts_line[:, 0], points))).T

    diffs = np.abs(np.subtract(np.array(ts_line)[:, 1], np.array(pwf_line)[:, 1]))
    max_ind = np.argmax(diffs)
    max_dist = diffs[max_ind]

    points = add_point(points, ts_line[max_ind], max_ind)
    return max_dist, points


@njit
def euclidean_dists(point: Tuple[float, float], line: np.ndarray) -> float:
    """
    Determines the distance between point and line
    """
    distances = np.zeros(len(line))
    for i in range(len(line)):
        point1 = line[i]
        distances[i] = ((point[0] - point1[0]) ** 2 + (point[1] - point1[1]) ** 2) ** 0.5
    return np.min(distances)


@njit
def add_point(points: np.ndarray, new_point: np.ndarray, new_idx: int) -> np.ndarray:
    """
    Adds point to points saving the order
    """
    new_points = np.zeros((points.shape[0] + 1, points.shape[1]))
    for i in range(points.shape[1]):
        new_points[0:points.shape[0], i] = points[:, i]
    new_points[points.shape[0], :] = np.array([new_point[0], new_point[1], new_idx])

    for i in range(new_points.shape[0] - 1):
        for j in range(i + 1, new_points.shape[0]):
            if new_points[j][0] < new_points[i][0]:
                p = np.copy(new_points[i, :])
                new_points[i, :] = new_points[j, :]
                new_points[j, :] = p

    return new_points


@njit
def get_pwf_line(x_ar, points) -> np.ndarray:
    """
    By base points returns the piecewise linear line
    """
    yy = np.zeros(len(x_ar))

    for ind in range(len(x_ar)):
        ind1 = 0
        for j in range(len(points[:, 0])):
            if points[j, 0] >= x_ar[ind]:
                ind1 = j
                break

        if ind != 0:
            yy[ind] = lin_f(points[ind1 - 1], points[ind1], x_ar[ind])
        elif ind == 0:
            yy[ind] = lin_f(points[ind1], points[ind1 + 1], x_ar[ind])
    return yy


@njit
def get_alpha(point1, point2) -> float:
    m = (point2[1] - point1[1]) / (point2[0] - point1[0])
    return m


@njit
def lin_f(point1, point2, x) -> float:
    m = get_alpha(point1, point2)
    c = point1[1] - m * point1[0]
    return m * x + c
