#!/usr/bin/env python3

import re
import subprocess
import sys
import logging


PACKAGE_NAME = 'puncher-a'
PKGVER_EXECUTABLE = '/usr/bin/pkgver.pl'
CURRENT_PUNCHER_VERSION_GETTER = ('dpkg-query', '-W', PACKAGE_NAME)

MONDATA_STATE_DIR = '/tmp/'
# String like '0.5' or 2.789266947.15aef6e68662366f6276bf68f16735e90c44a9ec
PREV_INSTALLED_VERSION_FILE = 'puncher_version__previous'

logger = logging.getLogger(__name__)

def get_saved_version():
    try:
        with open(MONDATA_STATE_DIR + PREV_INSTALLED_VERSION_FILE) as fd:
            return fd.read().strip()
    except FileNotFoundError:
        return None


def set_saved_version(version):
    with open(MONDATA_STATE_DIR + PREV_INSTALLED_VERSION_FILE, "w") as fd:
        fd.write(version)


def get_currently_installed_puncher_version():
    sp = subprocess.Popen(
        CURRENT_PUNCHER_VERSION_GETTER,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    retcode = sp.wait()
    if retcode != 0:
        return None
    # Format is like:
    # puncher-a	2.801046981.9db28d48a5b2dc22f6edfe9cc8f831ad8f82b8f4
    return sp.stdout.read().decode().split()[1]


def is_puncher_version_not_deployed_by_conductor(current_version):
    saved_version = get_saved_version()
    logger.debug("saved_version: %s", saved_version)

    sp = subprocess.Popen(
        (PKGVER_EXECUTABLE, ),
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    retcode = sp.wait(15)
    logger.info("%s exit code: %s", PKGVER_EXECUTABLE, retcode)

    # Typical output is:
    # vertis-ipv6checker, Installed: 0.5, Must be: 0.6
    res = []
    for line in sp.stdout:
        logger.debug("read from pkgver: %s", line)
        line = line.decode().strip()
        match = re.match(
            r"^([a-zA-Z0-9_-]+), Installed: ([^,]+), Must be: .+$", line)
        if not match:
            continue
        package, installed_version = match.groups()
        if package == PACKAGE_NAME:
            # Installed version should either be an older registered one
            # or None if there was none yet (new Puncher node).
            if saved_version not in (None, installed_version):
                logger.debug("found version", line)
                res.append(line)
            break
    else:
        logger.debug("No output from %s", PKGVER_EXECUTABLE)
        # Never broke, therefore no puncher-a version difference.
        # Either that or it is not installed yet.  In any case,
        # save whichever version is installed now.
        logger.debug("set_saved_version: %s", current_version)
        set_saved_version(current_version)
    return res


if __name__ == "__main__":
    logging.basicConfig(filename='/var/log/puncher_version.log', level=logging.DEBUG,
            format="%(asctime)s %(name)s - %(filename)s:%(lineno)d - %(funcName)s() - %(levelname)s - %(message)s")

    current_puncher_version = get_currently_installed_puncher_version()
    logger.info("current installed %s version: %s", PACKAGE_NAME, current_puncher_version)

    if current_puncher_version is None:
        # No puncher-a installed.  Its absense should be monitored elsewhere.
        print('PASSIVE-CHECK:puncher_version;0;OK')
        sys.exit(0)
    bad_versions = is_puncher_version_not_deployed_by_conductor(
        current_puncher_version)


    if bad_versions:
        logger.info("Bad version", bad_versions)
        print('PASSIVE-CHECK:puncher_version;2;Puncher bad version %s' % bad_versions)
    else:
        logger.info("Version OK")
        print('PASSIVE-CHECK:puncher_version;0;OK')

