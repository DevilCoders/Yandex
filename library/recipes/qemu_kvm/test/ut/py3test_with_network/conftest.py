import pytest
import subprocess
import logging

logging.basicConfig(level=logging.DEBUG)


@pytest.fixture(scope='session')
def logger(request):
    return logging.getLogger('test')


@pytest.fixture(scope='session')
def run(request, logger):
    def _run(args, check=True, stdin=subprocess.DEVNULL, **kwargs):
        logger.info("run '%s'", "' '".join(args))
        ret = subprocess.run(args, check=check, stdin=stdin, **kwargs)
        logger.info("run ".ljust(40, '-'))
        return ret
    return _run
