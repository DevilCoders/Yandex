# -*- coding: utf8 -*-
import io
import os
import re
import time

from PIL import Image


def url_to_filename(url, adblock_name):
    url_pattern = re.compile(r'(?:https?://)?(?:www\.)?([\w.\-/]*)?')
    split_url = url_pattern.match(url).group(1).replace('/', '_')[:50]
    return "_".join([split_url, adblock_name or "empty", str(time.time())])


def fullpage_screenshot(
    driver, logger, filename=None, adblock_name=None, callback_functions=None, wait_sec=3, scroll_count=0
):
    if filename is None:
        filename = '{0}.jpg'.format(url_to_filename(driver.current_url, adblock_name))

    _fullpage_screenshot(driver, logger, filename, callback_functions, wait_sec, scroll_count)
    logger.info("Full page screenshot saved on file '{}'".format(filename))
    return os.path.join(filename)


def _get_view_port_width_height(driver):
    screenshot = Image.open(io.BytesIO(driver.get_screenshot_as_png()))
    width, height = screenshot.size
    screenshot.close()
    return width, height


def _callback_functions(callback_functions, logger):
    if callback_functions:
        logger.info("Callback functions start")
        for function in callback_functions:
            function()
        logger.info("Callback functions done.")


def _get_page_total_width_height(driver, width=None, height=None):
    total_width = driver.execute_script(
        "return Math.max(document.body.scrollWidth, document.body.offsetWidth, "
        "document.documentElement.clientWidth, document.documentElement.scrollWidth, "
        "document.documentElement.offsetWidth);"
    )
    total_height = driver.execute_script(
        "return Math.max(document.body.scrollHeight, document.body.offsetHeight, "
        "document.documentElement.clientHeight, document.documentElement.scrollHeight,"
        " document.documentElement.offsetHeight);"
    )
    return width or total_width, height or total_height


def _get_offsets(total_width, total_height, viewport_width, viewport_height):
    """todo add cur_width if partners site width > viewport_width"""
    for cur_height in range(0, total_height, viewport_height):
        final_width, final_height = 0, cur_height
        if cur_height + viewport_height > total_height:
            final_height = max(0, total_height - viewport_height)
        yield final_width, final_height
    yield total_width, total_height


def _fullpage_screenshot(driver, logger, filename, callback_functions=None, wait_sec=3, scroll_count=0):
    """
    :param driver: current driver to take full screenshot
    :param filename: file name to save screenshot *.png
    :param callback_functions: функции которые будут выполняться перед каждым скриншотом
    :return: True if all is ok.
    """

    logger.info("Starting chrome full page screenshot workaround ...")
    viewport_width, viewport_height = _get_view_port_width_height(driver)
    total_width, total_height = _get_page_total_width_height(driver=driver, width=viewport_width)
    if scroll_count > 0:
        total_height = viewport_height * scroll_count
    stitched_image = Image.new('RGB', (total_width, total_height))

    logger.info("Total: ({}, {}), Viewport: ({},{})".format(total_width, total_height, viewport_width, viewport_height))
    driver.execute_script("window.scrollTo({}, {})".format(0, 0))
    for width, height in _get_offsets(total_width, total_height, viewport_width, viewport_height):
        logger.info("Scrolled To ({},{})".format(width, height))
        logger.info('Time sleep {}'.format(wait_sec))
        time.sleep(wait_sec)

        _callback_functions(callback_functions, logger)

        logger.info("Capturing screenshot ...")
        screenshot = Image.open(io.BytesIO(driver.get_screenshot_as_png()))
        stitched_image.paste(screenshot, (width, height))

        screenshot.close()
        driver.execute_script("window.scrollBy({}, {})".format(0, viewport_height))

    stitched_image.save(filename)
    logger.info("Finishing full page screenshot workaround...")
    return True
