"""
Crude implementation based on `subprocess` call of `ya` utility.
"""

from subprocess import check_output
import shlex


def yav(version, key):
    raw_secret = check_output(shlex.split(f'ya vault get version {version} --only-value {key}'))
    return raw_secret.decode('utf8').strip()
