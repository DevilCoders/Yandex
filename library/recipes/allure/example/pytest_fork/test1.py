import pytest
import logging


def test():
    with pytest.allure.step('step one XXX'):
        logging.info("step1")

    with pytest.allure.step('step two XXX'):
        logging.info("step2")
