import pytest
import logging


def test():
    with pytest.allure.step('step one'):
        logging.info("step1")

    with pytest.allure.step('step two'):
        logging.info("step2")
