import re

import markdown
from markdown.extensions import admonition

from mkdocs_yandex.ext import markdown as md_ext
from mkdocs_yandex.loggers import get_logger


class NoteBlockProcessor(admonition.AdmonitionProcessor):
    YFM_RE = re.compile(admonition.AdmonitionProcessor.RE.pattern.replace('!!!', r'\?\?\?'))
    log = get_logger(__name__)

    def test(self, parent, block):
        m = admonition.AdmonitionProcessor.RE.search(block)
        if m:
            raise md_ext.DeprecatedSyntaxError(
                'admonition syntax for notes', block,
                '{{% note {} {} %}}\n'
                '{}\n\n'
                '{{% endnote %}}'
                .format(m.group(1), '"{}"'.format(m.group(2)) or '', '\n'.join([l.lstrip() for l in block.splitlines()[1:]]))
            )
        return self.YFM_RE.search(block) or super(NoteBlockProcessor, self).test(parent, block)

    def run(self, parent, blocks):
        block = blocks.pop(0)
        m = self.YFM_RE.search(block)

        if m:
            heading = '' if m.group(2) is None else '"' + m.group(2) + '"'
            klass = m.group(1) or ''
            block = block.replace(m.group(0), '!!! {klass} {heading}\n'.format(klass=klass, heading=heading))
        super(NoteBlockProcessor, self).run(parent, [block] + blocks)


def makeExtension(**kwargs):
    return IncExtension(**kwargs)


class IncExtension(markdown.Extension):

    def extendMarkdown(self, md):
        note_blockprocessor = NoteBlockProcessor(md.parser)
        # instead of the admonition processor
        md.parser.blockprocessors.register(note_blockprocessor, 'admonition', 105)
