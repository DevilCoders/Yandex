"""Various cpu power related functoins"""


def cores_to_power(c):
    return c * 40.0


def power_to_cores(p):
    return p / 40.0
