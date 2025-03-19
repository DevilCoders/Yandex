import numpy as np
from pymorphy2 import MorphAnalyzer
import typing


class CachingMEREngine:
    """
    MER calculation engine.

    https://wiki.yandex-team.ru/cloud/ai/speechkit/stt/quality/mera/

    author: luda-gordeeva@
    """

    def __init__(
        self,
        features: typing.List[typing.Callable[[str, str], float]],
        features_weights: typing.List[typing.List[float]],
    ):
        assert len(features) == len(features_weights)
        self.features = features
        self.weights = np.array(features_weights, dtype=np.float64)
        self.cache = {}
        self.morphy = MorphAnalyzer()

    def __call__(self, hypothesis, reference):
        current_hash = str(hypothesis) + '_' + str(reference)
        if current_hash in self.cache:
            return self.cache[current_hash]

        result = self.calculate_metric(hypothesis, reference)
        self.cache[current_hash] = result
        return result

    def calculate_metric(self, text1: str, text2: str) -> float:
        score_function = lambda w1, w2: np.array([self._calc_features_for_words_pair(w1, w2)]).dot(self.weights)
        words1 = list(map(lambda x: self.morphy.parse(x)[0], text1.split()))
        words2 = list(map(lambda x: self.morphy.parse(x)[0], text2.split()))

        EMPTY_WORD = self.morphy.parse(" ")[0]
        n = len(words1)
        m = len(words2)

        if n == 0 and m == 0:
            return 0

        # calc dp
        dp = np.full((n + 1, m + 1), np.inf)
        dp[0, 0] = 0

        for i in range(n):
            for j in range(m):
                dp[i + 1, j] = min(dp[i + 1][j], dp[i][j] + score_function(words1[i], EMPTY_WORD))
                dp[i, j + 1] = min(dp[i][j + 1], dp[i][j] + score_function(EMPTY_WORD, words2[j]))
                dp[i + 1, j + 1] = min(dp[i + 1][j + 1], dp[i][j] + score_function(words1[i], words2[j]))

        for j in range(m):
            dp[n, j + 1] = min(dp[n, j + 1], dp[n, j] + score_function(EMPTY_WORD, words2[j]))

        for i in range(n):
            dp[i + 1, m] = min(dp[i + 1, m], dp[i, m] + score_function(words1[i], EMPTY_WORD))

        eps = 1e-10
        pairs = []
        i, j = n, m
        while i > 0 or j > 0:
            new_i, new_j = i, j
            pair = (EMPTY_WORD, EMPTY_WORD)
            if i > 0:
                if abs(dp[i - 1, j] + score_function(words1[i - 1], EMPTY_WORD) - dp[i, j]) < eps:
                    pair = (words1[i - 1], EMPTY_WORD)
                    new_i, new_j = i - 1, j
            if i > 0 and j > 0:
                if abs(dp[i - 1, j - 1] + score_function(words1[i - 1], words2[j - 1]) - dp[i, j]) < eps:
                    pair = (words1[i - 1], words2[j - 1])
                    new_i, new_j = i - 1, j - 1
            if j > 0:
                if abs(dp[i, j - 1] + score_function(EMPTY_WORD, words2[j - 1]) - dp[i, j]) < eps:
                    pair = (EMPTY_WORD, words2[j - 1])
                    new_i, new_j = i, j - 1

            pairs.append(pair)
            i, j = new_i, new_j

        mean_score = dp[n, m] / len(pairs)
        return float(1 / (1 + np.exp(-mean_score)))

    def _calc_features_for_words_pair(self, w1, w2):
        dists = np.zeros(len(self.features))
        for i in range(len(self.features)):
            dists[i] = self.features[i](w1, w2)
        return dists
