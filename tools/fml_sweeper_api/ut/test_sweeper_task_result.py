import pytest

from fml_sweeper_api.client import SweeperTaskResult


@pytest.fixture
def result_paraboloid():
    def f(x, y):
        return x**2 + y**2
    n_passes = 2
    result = {
        'target-function-definition': {
            'input-space': {'x': 'numeric', 'y': 'numeric'},
            'output-space': {'f': 'numeric', '-f': 'numeric'},
        },
        'passes': [{'points': []} for _ in range(n_passes)]
    }
    # split into 2 passes
    X = (-1, 0, 1)
    Y = (-1, 0, 1)
    for i, (x, y) in enumerate(zip(X, Y)):
        pass_idx = i % n_passes
        inputs = {'x': x, 'y': y}
        outputs = {'f': f(x, y), '-f': -f(x, y)}
        result['passes'][pass_idx]['points'].append({'inputs': inputs, 'outputs': outputs})
    return SweeperTaskResult(result)


class TestSweeperTaskResult:
    def test_wrong_min(self, result_paraboloid):
        with pytest.raises(ValueError):
            result_paraboloid.min('f2')

    def test_min(self, result_paraboloid):
        assert result_paraboloid.min('f') == 0

    def test_max(self, result_paraboloid):
        assert result_paraboloid.max('-f') == 0

    def test_argmin(self, result_paraboloid):
        argmin = result_paraboloid.argmin('f')
        assert len(argmin) == 1
        assert argmin[0] == {'x': 0, 'y': 0}

    def test_argmax(self, result_paraboloid):
        argmax = result_paraboloid.argmax('-f')
        assert len(argmax) == 1
        assert argmax[0] == {'x': 0, 'y': 0}

    def test_argmax_constraints(self, result_paraboloid):
        argmax = result_paraboloid.argmax('f', constraints=lambda i, o: i['x'] > 0 and i['y'] > 0)
        assert len(argmax) == 1
        assert argmax[0] == {'x': 1, 'y': 1}
