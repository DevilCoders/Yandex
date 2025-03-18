from jinja2.ext import Extension
from mkdocs_yandex.ext.jinja2 import reproduce_tag
from mkdocs_yandex.regex import RE_LIST, RE_LIST_END


class ListTag(Extension):
    tags = {'list'}

    def parse(self, parser):
        pass

    def preprocess(self, source, name, filename=None):
        res = reproduce_tag(source, RE_LIST, RE_LIST_END)
        return res
