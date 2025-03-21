# TODO: pymdownx should be in contrib

from __future__ import unicode_literals
from markdown import Extension
from markdown.inlinepatterns import SimpleTagInlineProcessor, DoubleTagInlineProcessor, SimpleTextInlineProcessor
from . import util

RE_SMART_CONTENT = r'((?:[^~]|~(?=[^\W_]|~|\s)|(?<=\s)~+?(?=\s))+?~*?)'
RE_CONTENT = r'((?:[^~]|(?<!~)~(?=[^\W_]|~))+?)'
RE_SMART_DEL = r'(?:(?<=_)|(?<![\w~]))(~{2})(?![\s~])%s(?<!\s)\1(?:(?=_)|(?![\w~]))' % RE_SMART_CONTENT
RE_DEL = r'(~{2})(?!\s)%s(?<!\s)\1' % RE_CONTENT

RE_SUB_DEL = r'(~{3})(?!\s)([^~]+?)(?<!\s)\1'
RE_SMART_SUB_DEL = r'(~{3})(?!\s)%s(?<!\s)\1' % RE_SMART_CONTENT
RE_SUB_DEL2 = r'(~{3})(?!\s)([^~]+?)(?<!\s)~{2}([^~ ]+?)~'
RE_SMART_SUB_DEL2 = r'(~{3})(?!\s)%s(?<!\s)~{2}(?:(?=_)|(?![\w~]))([^~ ]+?)~' % RE_SMART_CONTENT
RE_SUB = r'(~)([^~ ]+?|~)\1'

RE_NOT_TILDE = r'((^| )(~)( |$))'


class DeleteSubExtension(Extension):
    """Add delete and/or subscript extension to Markdown class."""

    def __init__(self, *args, **kwargs):
        """Initialize."""

        self.config = {
            'smart_delete': [True, "Treat ~~connected~~words~~ intelligently - Default: True"],
            'delete': [True, "Enable delete - Default: True"],
            'subscript': [True, "Enable subscript - Default: True"]
        }

        super(DeleteSubExtension, self).__init__(*args, **kwargs)

    def extendMarkdown(self, md):
        """Insert `<del>test</del>` tags as `~~test~~` and `<sub>test</sub>` tags as `~test~`."""

        config = self.getConfigs()
        delete = bool(config.get('delete', True))
        subscript = bool(config.get('subscript', True))
        smart = bool(config.get('smart_delete', True))

        escape_chars = []
        if delete or subscript:
            escape_chars.append('~')
        if subscript:
            escape_chars.append(' ')
        util.escape_chars(md, escape_chars)

        delete_rule = RE_SMART_DEL if smart else RE_DEL
        sub_del_rule = RE_SMART_SUB_DEL if smart else RE_SUB_DEL
        sub_del2_rule = RE_SMART_SUB_DEL2 if smart else RE_SUB_DEL2
        sub_rule = RE_SUB

        md.inlinePatterns.register(SimpleTextInlineProcessor(RE_NOT_TILDE), "not_tilde", 65)
        if delete:
            if subscript:
                md.inlinePatterns.register(DoubleTagInlineProcessor(sub_del_rule, "sub,del"), "sub_del", 64.9)
                md.inlinePatterns.register(DoubleTagInlineProcessor(sub_del2_rule, "sub,del"), "sub_del2", 64.8)

            # If not "smart", this needs to occur before `del`, but if "smart", this needs to be after `del`
            if subscript and not smart:
                md.inlinePatterns.register(SimpleTagInlineProcessor(sub_rule, "sub"), "sub", 64.8)

            md.inlinePatterns.register(SimpleTagInlineProcessor(delete_rule, "del"), "del", 64)

            # "smart", so this happens after `del`
            if subscript and smart:
                md.inlinePatterns.register(SimpleTagInlineProcessor(sub_rule, "sub"), "sub", 63.9)
        elif subscript:
            md.inlinePatterns.register(SimpleTagInlineProcessor(sub_rule, "sub"), "sub", 64.8)


def makeExtension(*args, **kwargs):
    """Return extension."""

    return DeleteSubExtension(*args, **kwargs)
