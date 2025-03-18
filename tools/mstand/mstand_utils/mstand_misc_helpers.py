# coding=utf-8

import logging
import os
import re
from typing import Optional

SCHEME_REGEXP_PATTERN = r'^[\w.\-+]+$'
SCHEME_REGEXP = re.compile(SCHEME_REGEXP_PATTERN)


def get_project_root_dir():
    utils_dir = os.path.dirname(__file__)
    root_dir = os.path.dirname(utils_dir)
    return root_dir


def get_project_path(rel_path):
    root_dir = get_project_root_dir()
    return os.path.join(root_dir, rel_path)


def extract_host_from_url(url: str, normalize: bool = False) -> Optional[str]:
    if not url:
        return None
    scheme_sep_idx = url.find("://")
    if scheme_sep_idx >= 0:
        scheme = url[:scheme_sep_idx]
        if SCHEME_REGEXP.match(scheme):
            url_no_scheme = url[scheme_sep_idx + 3:]
        else:
            url_no_scheme = url
    else:
        url_no_scheme = url

    min_sep_idx = None

    if normalize:
        separators = "/?#:"
    else:
        separators = "/?#"

    for sep in separators:
        sep_idx = url_no_scheme.find(sep)
        if sep_idx < 0:
            continue
        if min_sep_idx is None or sep_idx < min_sep_idx:
            min_sep_idx = sep_idx

    if min_sep_idx is not None:
        host = url_no_scheme[:min_sep_idx]
    else:
        host = url_no_scheme

    host = host.lower()
    if normalize:
        if host.startswith("www."):
            host = host[4:]
    return host


class SkipAction(object):
    def __init__(self, action, period=100000):
        self.action = action
        self.period = period
        self.current = 0

    def __call__(self):
        self.current += 1
        if self.current % self.period == 0:
            return self.action(self.current)
        else:
            return None


class SkipLogger(SkipAction):
    def __init__(self, message, period=100000):
        super(SkipLogger, self).__init__(lambda x: logging.info(message.format(x)),
                                         period=period)
