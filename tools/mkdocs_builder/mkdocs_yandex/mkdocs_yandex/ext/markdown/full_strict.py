import os

import markdown.extensions

import mkdocs.structure.pages as mkdocs_pages

from six.moves.urllib.parse import urlparse

from mkdocs_yandex.loggers import get_logger

logger = get_logger(__name__)


def makeExtension(**kwargs):
    return FullStrictExtension(**kwargs)


class FullStrictTreeprocessor(mkdocs_pages._RelativePathTreeprocessor):
    def path_to_url(self, url):
        super_url = super(FullStrictTreeprocessor, self).path_to_url(url)

        scheme, netloc, path, params, query, fragment = urlparse(url)

        if scheme or netloc or not path:
            return super_url

        target_path = os.path.join(os.path.dirname(self.file.src_path), path)
        target_path = os.path.normpath(target_path).lstrip(os.sep)

        if os.path.splitext(path)[1] == '':
            # Try adding .md extension if it's missing

            md_target_path = target_path + '.md'

            if md_target_path in self.files:
                logger.warn(
                    "'%s' contains a link to '%s'. There is .md file with the same name. Did you forget the extension?",
                    self.file.src_path, target_path
                )

        if not os.path.exists(os.path.join(os.path.dirname(self.file.abs_src_path), path)):
            logger.warn("'%s' contains a link to non existent '%s'", self.file.src_path, path)

        return super_url


class FullStrictExtension(markdown.extensions.Extension):
    def __init__(self, file, files):
        self.file = file
        self.files = files

    def extendMarkdown(self, md):
        proc = FullStrictTreeprocessor(self.file, self.files)
        md.treeprocessors._sort()
        priority = md.treeprocessors._priority[-1].priority - 5
        md.treeprocessors.register(proc, 'relpath', priority)
