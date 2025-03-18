import logging
import tempfile
from collections import namedtuple

import uatraits
import cachetools

import library.python.resource as rs

from antiadblock.cryprox.cryprox.config.system import UserDevice


UserAgentData = namedtuple('UserAgentData', ['BrowserName', 'isMobile', 'isRobot', 'device', 'isDeviceUndefined'])

browser_data_file = tempfile.NamedTemporaryFile()
browser_data_file.write(rs.find('/browser.xml'))
browser_data_file.flush()
DETECTOR = uatraits.detector(browser_data_file.name)


@cachetools.cached(cachetools.LRUCache(maxsize=10000))
def parse_user_agent_data(user_agent):
    res = dict()
    if user_agent:
        try:
            res = DETECTOR.detect(user_agent)
        except Exception:
            logging.debug('Failed to parse UA', action='parse_user_agent_data', exc_info=True)
    return UserAgentData(res.get("BrowserName", "Unknown"), res.get("isMobile", False), res.get("isRobot", False),
                         UserDevice.MOBILE if res.get("isMobile", False) else UserDevice.DESKTOP, "isMobile" not in res)


def get_ua_data_and_device(user_agent):
    ua_data = parse_user_agent_data(user_agent)
    device_type = "mobile" if ua_data.isMobile else "desktop"
    return ua_data, device_type
