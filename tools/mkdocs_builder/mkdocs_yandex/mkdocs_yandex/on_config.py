from mkdocs_yandex import run_context

from jinja2 import Environment
from jinja2 import FileSystemLoader

from mkdocs_yandex.ext.jinja2.cut import CutTag
from mkdocs_yandex.ext.jinja2.inc import IncTag
from mkdocs_yandex.ext.jinja2.note import NoteTag
from mkdocs_yandex.ext.jinja2.list import ListTag

from mkdocs_yandex.ext.markdown import md_ext_list
from mkdocs_yandex.ext.markdown import mdx_configs

from mkdocs_yandex.nav import preprocess_nav


def do(config, plugin_config):

    run_context.jinja2_env = Environment(
        loader=FileSystemLoader(config['docs_dir']),
    )

    if plugin_config['yfm20']:
        run_context.jinja2_env.comment_start_string = '{##'
        run_context.jinja2_env.comment_end_string = '##}'

        run_context.jinja2_env.add_extension(CutTag)
        run_context.jinja2_env.add_extension(IncTag)
        run_context.jinja2_env.add_extension(NoteTag)
        run_context.jinja2_env.add_extension(ListTag)

        config['markdown_extensions'] = md_ext_list

    if plugin_config['external_links']:
        try:
            p = config['markdown_extensions'].index('mkdocs_yandex.ext.markdown.list')
        except ValueError:
            p = 0
        config['markdown_extensions'].insert(p, 'mkdocs_yandex.ext.markdown.link')

    config['mdx_configs'] = mdx_configs

    run_context.config = config

    if plugin_config['separate_nav']:
        preprocess_nav()

    return config
