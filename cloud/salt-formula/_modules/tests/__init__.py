#! /usr/bin/python
"""init module for tests"""

from .yavpp_cores_test import GetCoresTest
from .yavpp_test import Bgp2vppTest

def __virtual__():
    """salt-specific flag to ignore this file"""
    return False
