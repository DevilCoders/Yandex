#!/usr/bin/python
"""yavpp _get_cores tests"""

import unittest
from yavpp import _get_cores as cores

NICS = [{
    'queues': 64,
    'numa_node': -1
}]

NICS_NUMA0 = [{
    'queues': 64,
    'numa_node': 0
}]

NICS_NUMA1 = [{
    'queues': 64,
    'numa_node': 1
}]


class GetCoresTest(unittest.TestCase):
    """yavpp _get_cores test class"""

    def test_2(self):
        """test 2-core vm distribution"""
        retval = cores(2, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('0', retval['gobgp_cores'])
        self.assertEqual('0', retval['controller_cores'])
        self.assertEqual('1', retval['main_cores'])
        self.assertNotIn('worker_cores', retval)
        self.assertNotIn('free_cores', retval)

    def test_4(self):
        """test 4-core vm distribution"""
        retval = cores(4, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('2', retval['gobgp_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('3', retval['main_cores'])
        self.assertNotIn('worker_cores', retval)
        self.assertNotIn('free_cores', retval)

    def test_6(self):
        """test 6-core vm distribution"""
        retval = cores(6, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('2', retval['gobgp_cores'])
        self.assertEqual('3', retval['main_cores'])
        self.assertEqual('4,5', retval['workers_cores'])
        self.assertNotIn('free_cores', retval)

    def test_8(self):
        """test 8-core vm distribution"""
        retval = cores(8, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('2,3', retval['gobgp_cores'])
        self.assertEqual('4', retval['free_cores'])
        self.assertEqual('5', retval['main_cores'])
        self.assertEqual('6,7', retval['workers_cores'])

    def test_10(self):
        """test 10-core vm distribution"""
        retval = cores(10, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('2,3', retval['gobgp_cores'])
        self.assertEqual('4', retval['free_cores'])
        self.assertEqual('5', retval['main_cores'])
        self.assertEqual('6-9', retval['workers_cores'])

    def test_12(self):
        """test 12-core vm distribution"""
        retval = cores(12, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('2,3', retval['gobgp_cores'])
        self.assertEqual('4-6', retval['free_cores'])
        self.assertEqual('7', retval['main_cores'])
        self.assertEqual('8-11', retval['workers_cores'])

    def test_14(self):
        """test 14-core vm distribution"""
        retval = cores(14, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('2,3', retval['gobgp_cores'])
        self.assertEqual('4', retval['free_cores'])
        self.assertEqual('5', retval['main_cores'])
        self.assertEqual('6-13', retval['workers_cores'])

    def test_16(self):
        """test 16-core vm distribution"""
        retval = cores(16, 1, NICS)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('2,3', retval['gobgp_cores'])
        self.assertEqual('4-6', retval['free_cores'])
        self.assertEqual('7', retval['main_cores'])
        self.assertEqual('8-15', retval['workers_cores'])

    def test_16_numa0(self):
        """test 16-core vm distribution whith 2 numas while nic at 0"""
        retval = cores(16, 2, NICS_NUMA0)
        self.assertEqual('8', retval['kernel_cores'])
        self.assertEqual('9', retval['controller_cores'])
        self.assertEqual('10,11', retval['gobgp_cores'])
        self.assertEqual('12-14', retval['free_cores'])
        self.assertEqual('15', retval['main_cores'])
        self.assertEqual('0-7', retval['workers_cores'])

    def test_16_numa1(self):
        """test 16-core vm distribution whith 2 numas while nic at 1"""
        retval = cores(16, 2, NICS_NUMA1)
        self.assertEqual('0', retval['kernel_cores'])
        self.assertEqual('1', retval['controller_cores'])
        self.assertEqual('2,3', retval['gobgp_cores'])
        self.assertEqual('4-6', retval['free_cores'])
        self.assertEqual('7', retval['main_cores'])
        self.assertEqual('8-15', retval['workers_cores'])



def __virtual__():
    """salt-specific flag to ignore this file"""
    return False
