from mkdocs.config import config_options
from mkdocs.plugins import BasePlugin

from mkdocs_yandex import on_config
from mkdocs_yandex import on_env
from mkdocs_yandex import on_files
from mkdocs_yandex import on_nav
from mkdocs_yandex import on_page_markdown
from mkdocs_yandex import on_post_build


class MkDocsYandex(BasePlugin):

    config_scheme = (
        ('separate_nav', config_options.Type(bool, default=True)),
        ('yfm20', config_options.Type(bool, default=True)),
        ('single_page', config_options.Type(bool, default=False)),
        ('single_pages_exclude', config_options.Type(list, default=None)),
        ('external_links', config_options.Type(bool, default=True)),
        ('full_strict', config_options.Type(bool, default=True)),
    )

    def on_config(self, config):
        dir_urls = 'use_directory_urls'

        if self.config['single_page'] and not self.config['yfm20']:
            raise config_options.ValidationError('Single page generation requires yfm 2.0 syntax')

        if self.config['single_page'] and config[dir_urls]:
            raise config_options.ValidationError('Single page generation requires %s=False' % dir_urls)

        if config['theme'].name in ['doccenter', 'daas'] and config[dir_urls]:
            raise config_options.ValidationError('%s theme requires %s=False' % (config['theme'].name, dir_urls))

        return on_config.do(config, self.config)

    def on_pre_build(self, config):
        pass

    def on_files(self, files, config):
        return on_files.do(files, config)

    def on_nav(self, nav, config, files):
        on_nav.do(nav)

    def on_env(self, env, config, files):
        return on_env.do(env)

    def on_page_markdown(self, markdown, page, config, files):
        return on_page_markdown.do(markdown, page, config, files, self.config)

    def on_pre_page(self, page, config, files):
        pass

    def on_page_context(self, context, page, config, nav):
        pass

    def on_page_content(self, content, page, config, files):
        pass

    def on_post_build(self, config):
        return on_post_build.do(self.config)
