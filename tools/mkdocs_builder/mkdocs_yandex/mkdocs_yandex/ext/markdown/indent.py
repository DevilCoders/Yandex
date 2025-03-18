"""
Sets the default indentation to 2 (instead of 4) for lists.

https://github.com/Python-Markdown/markdown/blob/3.0.1/markdown/core.py#L86

https://python-markdown.github.io/#differences

https://guides.github.com/features/mastering-markdown/#syntax

https://help.github.com/articles/basic-writing-and-formatting-syntax/#nested-lists
"""

from markdown.blockprocessors import ListIndentProcessor
from markdown.blockprocessors import OListProcessor
from markdown.blockprocessors import UListProcessor

from markdown.extensions import Extension


TAB_LENGTH = 2


class ShortIndentExtension(Extension):
    def extendMarkdown(self, md):
        original_tab_length = md.parser.md.tab_length
        md.parser.md.tab_length = TAB_LENGTH

        try:
            md.parser.blockprocessors.register(ListIndentProcessor(md.parser), 'indent', 90)
            md.parser.blockprocessors.register(OListProcessor(md.parser), 'olist', 40)
            md.parser.blockprocessors.register(UListProcessor(md.parser), 'ulist', 30)
        finally:
            md.parser.md.tab_length = original_tab_length


def makeExtension(*args, **kwargs):
    return ShortIndentExtension(*args, **kwargs)
