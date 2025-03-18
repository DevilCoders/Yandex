import os
import pytest
import six
import xml.etree.ElementTree as etree

import yatest.common as yac

import jinja2
import markdown
import mkdocs.config

from mkdocs_yandex import on_page_markdown
from mkdocs_yandex import plugin as ya_plugin
from mkdocs_yandex import run_context
from mkdocs_yandex.ext import markdown as md_ext
from mkdocs_yandex.ext.markdown import inc as markdown_inc
from mkdocs_yandex.loggers import warning_counter, warning_filter, get_warning_counts

DATA_DIR = yac.source_path('tools/mkdocs_builder/mkdocs_yandex/tests/data')


def load_config(base_dir='test', **cfg):
    """ Helper to build a simple config for testing. """
    path_base = os.path.join(DATA_DIR, base_dir)
    cfg = cfg or {}
    if 'site_name' not in cfg:
        cfg['site_name'] = 'Example'
    if 'config_file_path' not in cfg:
        cfg['config_file_path'] = os.path.join(path_base, 'mkdocs.yml')
    if 'docs_dir' not in cfg:
        cfg['docs_dir'] = os.path.join(path_base, 'docs')
    if 'site_dir' not in cfg:
        cfg['site_dir'] = yac.work_path('site')
    conf = mkdocs.config.load_config(config_file=cfg['config_file_path'], **cfg)

    return conf


class TestMkdocsPlugin(object):

    @pytest.mark.parametrize('toc_name', ['toc.yml', 'toc_nested.yml'])
    def test_event_on_config(self, toc_name):
        plugin = ya_plugin.MkDocsYandex()
        plugin.load_config({'separate_nav': True})
        config = load_config(config_file_path=os.path.join(DATA_DIR, 'toc', toc_name), base_dir='toc')

        result = plugin.on_config(config)
        return result['nav']


class TestExtensions(object):

    def setup(self):
        self.base_path = os.path.join(DATA_DIR, 'ext')

        self.jinja = jinja2.Environment(
            loader=jinja2.FileSystemLoader(self.base_path),
            comment_start_string='{##',
            comment_end_string='##}',
        )

        self.markdown = markdown.Markdown(
            extensions=[
                'markdown.extensions.admonition',
                'markdown.extensions.tables',
            ],
        )

        # lets test them as isolated as possible
        self.jinja_ext = {
            'cut': [
                'mkdocs_yandex.ext.jinja2.cut.CutTag',
                'mkdocs_yandex.ext.jinja2.list.ListTag',
            ],
            'inc': ['mkdocs_yandex.ext.jinja2.inc.IncTag'],
            'inc_note': [
                'mkdocs_yandex.ext.jinja2.inc.IncTag',
                'mkdocs_yandex.ext.jinja2.note.NoteTag',
            ],
            'inc_yaml': ['mkdocs_yandex.ext.jinja2.inc.IncTag'],
            'inc_error_yaml': ['mkdocs_yandex.ext.jinja2.inc.IncTag'],
            'list': [
                'mkdocs_yandex.ext.jinja2.list.ListTag',
            ],
        }

        self.md_ext = {
            'anchor': [
                'mkdocs_yandex.ext.markdown.anchor',
                'mkdocs_yandex.ext.markdown.attr_list',
            ],
            'cut': [
                'mkdocs_yandex.ext.markdown.cut',
                'mkdocs_yandex.ext.markdown.list',
            ],
            'inc': [
                'mkdocs_yandex.ext.markdown.inc',
                'markdown.extensions.fenced_code',
                'mkdocs_yandex.ext.markdown.attr_list',
            ],
            'inc_note': [
                'mkdocs_yandex.ext.markdown.inc',
                'mkdocs_yandex.ext.markdown.note',
            ],
            'inc_yaml': [
                'mkdocs_yandex.ext.markdown.inc',
                'mkdocs_yandex.ext.markdown.attr_list',
            ],
            'inc_error_yaml': [
                'mkdocs_yandex.ext.markdown.inc',
                'mkdocs_yandex.ext.markdown.attr_list',
            ],
            'list': [
                'mkdocs_yandex.ext.markdown.list',
            ],
            'list_indent': [
                'mkdocs_yandex.ext.markdown.list',
                'mkdocs_yandex.ext.markdown.indent',
            ],
            'list_fenced': [
                'mkdocs_yandex.ext.markdown.list',
                'mkdocs_yandex.ext.markdown.indent',
                'mkdocs_yandex.ext.markdown.pymdownx.superfences',
            ],
            'tasklist': ['mkdocs_yandex.ext.markdown.list'],
            'tasklist_indent': [
                'mkdocs_yandex.ext.markdown.list',
                'mkdocs_yandex.ext.markdown.indent',
            ],
        }

        run_context.jinja2_env = self.jinja
        run_context.config = {'docs_dir': self.base_path}

    @pytest.mark.parametrize('md_path', ['note'])
    def test_jinja2(self, md_path):

        self.jinja.add_extension('mkdocs_yandex.ext.jinja2.note.NoteTag')

        return self.jinja.get_template(md_path + '.md').render()

    @pytest.mark.parametrize('md_name', ['cut', 'inc', 'inc_note', 'list', 'list_indent', 'list_fenced', 'tasklist', 'tasklist_indent'])
    def test_markdown(self, md_name):
        for e in self.jinja_ext.get(md_name, []):
            self.jinja.add_extension(e)

        self.markdown.registerExtensions(extensions=self.md_ext.get(md_name, []), configs={})

        md_path = os.path.join(self.base_path, md_name + '.md')
        with open(md_path, 'r') as f:
            md = f.read()

        run_context.file_being_processed = run_context.page_file_abs_path = md_path

        return self.markdown.reset().convert(md)

    def test_markdown_anchor(self):
        name = 'anchor'
        md_name = 'index.md'
        for e in self.jinja_ext.get(name, []):
            self.jinja.add_extension(e)

        self.markdown.registerExtensions(extensions=self.md_ext.get(name, []), configs={})

        md_path = os.path.join(DATA_DIR, 'anchor', 'docs', md_name)
        with open(md_path, 'r') as f:
            md = f.read()

        expected_path = yac.work_path(md_name)

        # some ugly monkey patching in tests has never killed anybody
        class C(object):
            pass

        def get_file_from_path(x):
            r = C()
            r.abs_dest_path = expected_path
            return r

        run_context.files = C()
        run_context.files.get_file_from_path = get_file_from_path

        run_context.anchors = {}
        run_context.file_being_processed = run_context.page_file_abs_path = md_path
        run_context.page_src_path = md_name

        self.markdown.reset().convert(md)
        result = dict((os.path.relpath(kv[0], yac.work_path()), ''.join(kv[1]['text'])) for kv in six.iteritems(run_context.anchors))
        assert result == {
            md_name: 'heading',
            md_name + '#id1': 'heading',
            md_name + '#id2': 'Some code',
            md_name + '#id_3': 'Some more',
            md_name + '#4': 'And strong heading',
            md_name + '#5_id': 'Simple',
        }

    def test_markdown_inc_error(self):
        md_name = 'inc_error_yaml'
        for e in self.jinja_ext.get(md_name, []):
            self.jinja.add_extension(e)

        self.markdown.registerExtensions(extensions=self.md_ext.get(md_name, []), configs={})

        md_path = os.path.join(self.base_path, md_name + '.md')

        with open(md_path, 'r') as f:
            md = f.read()

        run_context.file_being_processed = run_context.page_file_abs_path = md_path
        if warning_counter:
            warning_counter.counts.clear()
        else:
            warning_filter.count = 0

        result = ''.join(etree.fromstring('<root>' + self.markdown.reset().convert(md) + '</root>').itertext())

        assert 'Expect error\n\n\n' == result

        assert 2 == get_warning_counts()

    def test_markdown_inc_params(self):
        md_name = 'inc_yaml'
        for e in self.jinja_ext.get(md_name, []):
            self.jinja.add_extension(e)

        self.markdown.registerExtensions(extensions=self.md_ext.get(md_name, []), configs={})

        md_path = os.path.join(self.base_path, md_name + '.md')
        with open(md_path, 'r') as f:
            md = f.read()

        run_context.file_being_processed = run_context.page_file_abs_path = md_path

        result = ''.join(etree.fromstring('<root>' + self.markdown.reset().convert(md) + '</root>').itertext())
        assert '''Expected: line 1
* line 1:

Expected: lines 1 and 5
* line 1:
    * line 5: bla-bla

Expected: lines 1-3
* line 1:
    * line 2: bla-bla
    * line 3:

Expected: lines 6-8
* line 6: bla-bla
* line 7:
    * line 8: bla-bla

Expected: lines 1, 3-4, 7-8
* line 1:
    * line 3:
        * line 4: bla-bla
* line 7:
    * line 8: bla-bla

Expected: error


Expected: lines 7, 8
* line 7:
    * line 8: bla-bla

Expected: line 3
* line 3:

''' == result

    def test_sanitize_html_ok(self):
        with open(os.path.join(self.base_path, 'list.md'), 'r') as f:
            md = f.read()
        on_page_markdown.sanitize_html(md)

    def test_sanitize_html_bad(self):
        with open(os.path.join(self.base_path, 'bad_list.md'), 'r') as f:
            md = f.read()
        with pytest.raises(md_ext.DeprecatedSyntaxError) as e:
            on_page_markdown.sanitize_html(md)
        assert e.value.subject == '<details> html tag'

        with open(os.path.join(self.base_path, 'bad_anchor.md'), 'r') as f:
            md = f.read()
        with pytest.raises(md_ext.DeprecatedSyntaxError) as e:
            on_page_markdown.sanitize_html(md)
        assert e.value.subject == '<a name="..."> html tag'


class TestUnindent(object):
    class Parser(object):
        class Md(object):
            def __init__(self):
                self.tab_length = None

        def __init__(self):
            self.md = self.Md()

    def test_unindent_on_spaces_no_spaces_before(self):
        input = [
            'int a = 3;',
            '',
            'int b = 4;']

        out_1 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input)
        out_2 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input, True)

        assert out_1 == out_2
        assert input == out_2

    def test_unindent_on_spaces_with_spaces_before(self):
        input = [
            '  int a = 3;',
            '',
            '  if (a == 3) {',
            '     int b = 4;',
            '',
            '     a = b;',
            '  }',
            '']

        out_1 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input)
        out_2 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input, True)

        assert input == out_1
        assert out_2 == [
            'int a = 3;',
            '',
            'if (a == 3) {',
            '   int b = 4;',
            '',
            '   a = b;',
            '}',
            '']

    def test_unindent_on_tabs_no_tabs_before(self):
        input = [
            'a := 3',
            '',
            'b := 4']

        out_1 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input)
        out_2 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input, True)

        assert out_1 == out_2
        assert input == out_2

    def test_unindent_on_tabs_with_tabs_before(self):
        input = [
            '\ta := 3',
            '',
            '\tif a == 3 {',
            '\t\tb := 4',
            '',
            '\t\ta = b',
            '\t}',
            '']

        out_1 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input)
        out_2 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input, True)

        assert input == out_1
        assert out_2 == [
            'a := 3',
            '',
            'if a == 3 {',
            '\tb := 4',
            '',
            '\ta = b',
            '}',
            '']

    def test_unindent_no_first_line_has_minimal_spaces(self):
        input = [
            '',
            '',
            '    int a = 3;',
            '  int b = 3;',
            '',
            '    int c = 3;',
            '   int d = 4',
            '']

        out_1 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input)
        out_2 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input, True)

        assert out_1 == input
        assert out_2 == [
            '',
            '',
            '  int a = 3;',
            'int b = 3;',
            '',
            '  int c = 3;',
            ' int d = 4',
            '']

    def test_unindent_not_first_line_has_no_spaces(self):
        input = [
            '',
            '',
            '    int a = 3;',
            '  int b = 3;',
            '',
            '    int c = 3;',
            '   int d = 4',
            'int e = 5;',
            '']

        out_1 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input)
        out_2 = markdown_inc.IncBlockProcessor(self.Parser()).remove_code_indents(input, True)

        assert out_1 == input
        assert out_2 == input
