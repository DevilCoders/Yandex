from collections import namedtuple

SuspInfo = namedtuple('SuspInfo',
        (
            'coeff',    # tweak level multiplier (default is 1)
            'name',     # name of a tweak
            'descr'     # decision description
        )
    )
