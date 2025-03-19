import numpy as np
import numba as nb

@nb.njit
def _slope(ts):
    if len(ts)>3:
        return np.nanmean(ts[len(ts)//2:])- np.nanmean(ts[:len(ts)//2])
    else: 
        return 0


def slope(ts):
    return _slope(ts.values)

def abs_energy(ts):
    return np.nansum(ts.values**2)