"""
    Update path to packages, adding path to venv installed packages
"""

import os
import sys

_packages_path = os.path.abspath(os.path.join(os.path.dirname(__file__), 'venv/venv/lib/python2.7/site-packages'))

if not os.path.exists(_packages_path):
    # we can sometimes have old Python
    _packages_path = os.path.abspath(os.path.join(os.path.dirname(__file__), 'venv/venv/lib/python2.6/site-packages'))
if not os.path.exists(_packages_path):
    # At the moment we allow skynet Python
    # raise Exception('gencfg is not properly installed! Please, run ./install.sh to install gencfg.')
    pass
else:
    sys.path.insert(0, _packages_path)
