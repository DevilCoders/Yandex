import io
import logging
import os
import re
import sys

import jinja2
import jinja2.exceptions
from mkdocs import utils as mkdocs_utils
from yaml import parser as yaml_parser

from mkdocs_yandex.ext.jinja2 import inc, note, list, cut


from . import util

logger = logging.getLogger(__name__)

HREF_RE = re.compile('href="(http|https)://([^"]+)"')
INNER_LINK_RE = re.compile('\]\((?!https?:)([^\)#]+)\)')
INNER_LINK_ONLY_ANCHOR_RE = re.compile('\]\(#([^\)]+)\)')
INNER_LINK_WITH_ANCHOR_RE = re.compile('\]\((?!http)([^#]{2,})#([^\)]+)\)')
A_NAME_RE = re.compile('<a name="([^"]+)">')


def yaml_load(config_path):
    try:
        with io.open(config_path, 'r', encoding='utf-8') as f:
            cfg_dict = mkdocs_utils.yaml_load(f)
    except Exception as e:
        if isinstance(e, yaml_parser.ParserError) and any([e.context_mark.line, e.problem_mark.line]):

            with io.open(config_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            msg = []
            for m in [e.context_mark, e.problem_mark]:
                if m.line:
                    msg.append(lines[m.line - 1])
            msg = '\n'.join(msg)
            logger.warn('\nFailed to load preprocessed config near these lines:\n%s\nOriginal error:\n%s\n%s', msg, e.context, e.problem)
            sys.exit(1)
        else:
            with io.open(config_path, 'r', encoding='utf-8') as f:
                logger.warn('Failed to load preprocessed config:\n%s', f.read())
            raise
    return cfg_dict


class Preprocessor(object):
    def __init__(self, docs_dir, yfm_enabled):
        if yfm_enabled:
            self.env = jinja2.Environment(
                loader=jinja2.FileSystemLoader(docs_dir),
                comment_start_string='{##',
                comment_end_string='##}',
            )
            self.env.add_extension(cut.CutTag)
            self.env.add_extension(inc.IncTag)
            self.env.add_extension(note.NoteTag)
            self.env.add_extension(list.ListTag)
        else:
            self.env = jinja2.Environment(
                loader=jinja2.FileSystemLoader(docs_dir),
            )

    def template(self, content):
        return self.env.from_string(content)

    # TODO try to replace with plugin
    def preprocess_markdown(self, file_path, markdown_args, output_path=None):
        """Preformating given files with configuration 'extra' options"""
        logger.debug('Preprocess markdown for {}'.format(file_path))

        try:
            with io.open(file_path, 'r', encoding='utf-8') as f:
                original_markdown = self.template(f.read())

            if output_path:
                util.ensure_dir(os.path.dirname(output_path))
            with io.open(output_path or file_path, 'w', encoding='utf-8') as f:
                res = original_markdown.render(**markdown_args)
                f.write(res)
        except jinja2.exceptions.TemplateSyntaxError as e:
            source_lines = e.source.splitlines()
            logger.error(
                'Failed to preprocess file %s\nNear: %s\nYou can find full template in verbose mode logs',
                file_path,
                source_lines[e.lineno - 1],
            )
            logger.debug('\n' + '\n'.join(['{}|\t{}'.format(kv[0] + 1, kv[1]) for kv in enumerate(source_lines)]))
            raise
        except Exception:
            logger.error('Failed to preprocess file %s', file_path)
            raise


# TODO try to replace with plugin
def preprocess_markdown_for_single_page(top_anchor, content, need_additional_anchor=True):
    """Some strange patching"""
    lines = []
    is_raw = False  # code content
    for line in content.split('\n'):
        if line.startswith('```'):
            is_raw = False if is_raw else True
        if line.startswith('#') and not is_raw:
            anchor = line.strip(' #').replace(' ', '-').lower()
            if top_anchor:
                anchor = '--'.join([top_anchor, anchor])
            if need_additional_anchor:
                lines.append('<a name="%s"></a>' % anchor)
        line = re.sub(INNER_LINK_ONLY_ANCHOR_RE, '](#%s--\\1)' % top_anchor if top_anchor else '](#\\1)', line)
        line = re.sub(INNER_LINK_WITH_ANCHOR_RE, '](#\\1--\\2)', line)
        line = re.sub(INNER_LINK_RE, '](#\\1)', line)
        line = re.sub(A_NAME_RE, '<a name="%s--\\1">' % top_anchor if top_anchor else '<a name="\\1">', line)
        line = line.replace('../', '').replace('.md)', ')').replace('.md--', '--')
        lines.append(line)

    return '\n'.join(lines)


def postprocess_html(abs_path):
    lines = []
    with io.open(abs_path, 'r', encoding='utf-8') as f:
        for line in f.readlines():
            for match in re.finditer(HREF_RE, line):
                if 'yandex-team.ru' not in match.group(2):
                    to_replace = match.group(1) + '://' + match.group(2)
                    line = line.replace(to_replace, 'https://h.yandex-team.ru/?' + to_replace, 1)
            lines.append(line)
    with io.open(abs_path, 'w', encoding='utf-8') as f:
        f.write(''.join(lines))
