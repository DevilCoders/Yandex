import jinja2.ext
from mkdocs_yandex import regex
from . import reproduce_tag


class CutTag(jinja2.ext.Extension):
    tags = {'cut'}

    def parse(self, parser):
        pass

    def preprocess(self, source, name, filename=None):
        res = reproduce_tag(source, regex.RE_CUT, regex.RE_CUT_END)
        return res
