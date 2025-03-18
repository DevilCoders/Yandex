import re

from markdown.blockprocessors import BlockProcessor
from markdown.extensions import Extension
from mkdocs_yandex.regex import RE_LIST, RE_LIST_END


def makeExtension(**kwargs):
    return ListExtension(**kwargs)


class ListExtension(Extension):

    def __init__(self, **kwargs):
        self.config = {}
        self.RE = re.compile(RE_LIST)
        super(ListExtension, self).__init__(**kwargs)

    # Should be implemented
    def extendMarkdown(self, md):
        md.registerExtension(self)
        i = md.parser.blockprocessors.get_index_for_name('indent')
        before = md.parser.blockprocessors._priority[i].priority
        if i < len(md.parser.blockprocessors) - 1:
            after = md.parser.blockprocessors._priority[i + 1].priority
        else:
            # location is last item
            after = before - 10
        priority = before - ((before - after) / 2)
        md.parser.blockprocessors.register(ListBlockProcessor(md.parser, self), 'list_block_processor', priority)


class ListBlockProcessor(BlockProcessor):

    def __init__(self, parser, extension):
        super(ListBlockProcessor, self).__init__(parser)
        self.RE_START = re.compile(RE_LIST)
        self.RE_END = re.compile(RE_LIST_END)

    def test(self, parent, block):
        return bool(self.RE_START.match(block)) or self.RE_END.match(block)

    def run(self, parent, blocks):
        block = blocks.pop(0)

        m = self.RE_START.search(block)
        if m:
            theclass = ' class="' + m.group(2) + '"' if m.group(2) else ''
            blocks[0:0] = ['<div{}>'.format(theclass)]

        m = self.RE_END.search(block)
        if m:
            blocks[0:0] = ['</div>']
