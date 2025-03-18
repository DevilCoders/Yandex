from six.moves.urllib.parse import urlparse

from markdown import Extension
from markdown.inlinepatterns import LinkInlineProcessor
from markdown.inlinepatterns import LINK_RE


def makeExtension(**kwargs):
    return Link2Extension(**kwargs)


class Link2InlineProcessor(LinkInlineProcessor):

    def handleMatch(self, m, data):
        el, start, end = super(Link2InlineProcessor, self).handleMatch(m, data)

        if el is None:
            return None, None, None

        if el.tag == 'a':
            url = el.get('href')
        elif el.tag == 'img':
            url = el.get('src')
        else:
            return el, start, end

        scheme, netloc, path, params, query, fragment = urlparse(url)

        if scheme:
            el.set('target', '_blank')

        return el, start, end


class Link2Extension(Extension):

    def extendMarkdown(self, md):
        link2_pattern = Link2InlineProcessor(LINK_RE, md)
        md.inlinePatterns.register(link2_pattern, 'link', 160)
