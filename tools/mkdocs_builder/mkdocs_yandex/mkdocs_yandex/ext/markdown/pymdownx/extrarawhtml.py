# TODO: pymdownx should be in contrib

from __future__ import unicode_literals
from markdown import Extension
try:
    from markdown.extensions.md_in_html import MarkdownInHtmlProcessor
except ImportError:
    from markdown.extensions.extra import MarkdownInHtmlProcessor
import re


class ExtraRawHtmExtension(Extension):
    """Add raw HTML extensions to Markdown class."""

    def extendMarkdown(self, md):
        """Register extension instances."""

        md.registerExtension(self)
        # Turn on processing of markdown text within raw html
        md.preprocessors['html_block'].markdown_in_raw = True
        md.parser.blockprocessors.register(
            MarkdownInHtmlProcessor(md.parser), 'markdown_block', 105
        )
        md.parser.blockprocessors.tag_counter = -1
        md.parser.blockprocessors.contain_span_tags = re.compile(
            r'^(p|h[1-6]|li|dd|dt|td|th|legend|address)$',
            re.IGNORECASE
        )


def makeExtension(*args, **kwargs):
    """Return extension."""

    return ExtraRawHtmExtension(*args, **kwargs)
