import datetime
import logging
import os
from pathlib import Path, PosixPath
from typing import Dict

from jinja2 import BaseLoader, TemplateNotFound


def load_file(fp: PosixPath) -> str:
    if fp.as_posix().find("dashboards/") != -1:
        return fp.read_text()
    return fp.read_text().replace("\n", "\\n")


def build_key(fp: str) -> str:
    idx = fp.find("alerts/")
    if idx != -1:
        return fp[idx:]

    idx = fp.find("graphs/")
    if idx != -1:
        return fp[idx:]

    idx = fp.find("dashboards/")
    if idx != -1:
        return fp[idx:]

    raise Exception("Unknown path: {}".format(fp))


def load_templates(dir_path) -> Dict:
    result = {}
    if "YATEST" in os.environ.keys() and os.environ["YATEST"] == "1":
        from yatest.common import source_path
        path = source_path(dir_path)
    else:
        path = dir_path

    for fp in Path(path).rglob('*.t.js'):
        result[build_key(fp.as_posix()[:-5])] = load_file(fp)  # remove .t.js from the file name
    for fp in Path(path).rglob('*.t.j2'):
        result[build_key(fp.as_posix()[:-5])] = load_file(fp)  # remove .t.j2 from the file name
    return result


class SolomonLoader(BaseLoader):

    def __init__(self, path):
        self.path = path
        self.templates = load_templates(path)
        logging.info("Loaded: %d templates for %s", len(self.templates), path)
        logging.debug('templates %s', self.templates)

    def get_source(self, environment, template):
        if template not in self.templates.keys():
            raise TemplateNotFound(template)
        return self.templates[template], "{}.t.js".format(template), datetime.datetime.now

    def get_keys(self):
        return self.templates.keys()
