import re

import markdown
from markdown.extensions import admonition

from mkdocs_yandex import regex
from mkdocs_yandex.loggers import get_logger


class CutBlockProcessor(admonition.AdmonitionProcessor):
    log = get_logger(__name__)

    def __init__(self, parser, extension):
        super(CutBlockProcessor, self).__init__(parser)
        self.RE_START = re.compile(regex.RE_CUT)
        self.RE_END = re.compile(regex.RE_CUT_END)

    def test(self, parent, block):
        return bool(self.RE_START.match(block)) or self.RE_END.match(block)

    def run(self, parent, blocks):
        block = blocks.pop(0)

        m = self.RE_START.search(block)
        if m:
            title = m.group('title')
            blocks[0:0] = ['<div class="details" is-cat=1 cut-title="{}">'.format('Read more' if title is None else title.replace('"', ''))]

        m = self.RE_END.search(block)
        if m:
            blocks[0:0] = ['</div>']


def makeExtension(**kwargs):
    return CutExtension(**kwargs)


class CutExtension(markdown.Extension):

    def __init__(self, **kwargs):
        self.config = {}
        self.RE = re.compile(regex.RE_CUT)
        super(CutExtension, self).__init__(**kwargs)

    def extendMarkdown(self, md):
        i = md.parser.blockprocessors.get_index_for_name('indent')
        before = md.parser.blockprocessors._priority[i].priority
        if i < len(md.parser.blockprocessors) - 1:
            after = md.parser.blockprocessors._priority[i + 1].priority
        else:
            # location is last item
            after = before - 10
        priority = before - ((before - after) / 2)
        md.parser.blockprocessors.register(CutBlockProcessor(md.parser, self), 'cut_block_processor', priority)
