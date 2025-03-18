from .utils import BaseSerializable, is_array


# TODO support all kinds of sweeper passes
class GridSearchSweeperPass(BaseSerializable):
    def __init__(self, grid):
        """

        :param grid: mapping from axis name to iterable or scalar
        """
        self._grid = grid

    def to_dict(self):
        d = {"sweep-type": "grid-search", "sweep-scheme": {}}
        for axis_name, value in self._grid.iteritems():
            if is_array(value):
                d["sweep-scheme"][axis_name] = {
                    "axis-type": "sequence",
                    "elements": value,
                    "weights": [1. for _ in value],
                }
            else:
                d["sweep-scheme"][axis_name] = {
                    "axis-type": "constant",
                    "axis-value": value
                }

        return d
