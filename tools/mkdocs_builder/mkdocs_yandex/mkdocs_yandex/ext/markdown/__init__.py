from mkdocs_yandex.ext.markdown import slugs

md_ext_list = [
    'markdown.extensions.admonition',
    'markdown.extensions.codehilite',
    'markdown.extensions.tables',
    'markdown.extensions.toc',
    'markdown.extensions.meta',
    #
    'mkdocs_yandex.ext.markdown.note',
    'mkdocs_yandex.ext.markdown.attr_list',
    'mkdocs_yandex.ext.markdown.inc',
    'mkdocs_yandex.ext.markdown.indent',
    # 'mkdocs_yandex.ext.markdown.link',
    'mkdocs_yandex.ext.markdown.cut',
    'mkdocs_yandex.ext.markdown.list',
    'mkdocs_yandex.ext.markdown.anchor',
    #
    'mkdocs_yandex.ext.markdown.pymdownx.extrarawhtml',
    'mkdocs_yandex.ext.markdown.pymdownx.superfences',
    'mkdocs_yandex.ext.markdown.pymdownx.tasklist',
    'mkdocs_yandex.ext.markdown.pymdownx.tilde',
]

mdx_configs = {
    'markdown.extensions.toc': {
        'slugify': slugs.slugify,
        'permalink': '#',
    }
}


class DeprecatedSyntaxError(Exception):
    def __init__(self, subject, where, replacement):
        self.subject = subject
        self.where = where
        self.replacement = replacement

    def __str__(self):
        return \
            'Using of {subject} is not allowed in \n{where}\n\n'\
            'Please use Yandex flavoured Markdown syntax instead:\n'\
            '{replacement}\n'\
            'See https://wiki.yandex-team.ru/yfm'\
            .format(subject=self.subject, where=self.where, replacement=self.replacement)


# preprocessors -> block processors -> inline processors -> tree processors -> post processors
