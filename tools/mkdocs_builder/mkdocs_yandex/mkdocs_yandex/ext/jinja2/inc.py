from jinja2.ext import Extension
from mkdocs_yandex.ext.jinja2 import reproduce_tag
from mkdocs_yandex.regex import RE_INC, RE_CODE


class IncTag(Extension):

    def parse(self, parser):
        pass

    def preprocess(self, source, name, filename=None):
        res = reproduce_tag(source, RE_INC)
        res = reproduce_tag(res, RE_CODE)
        return res
