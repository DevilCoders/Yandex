# coding=utf-8

import textwrap

import jinja2
import markdown
import pytest
import six

from mkdocs_yandex import run_context
from mkdocs_yandex.ext import markdown as md_ext


TEST_CASES = {
    'simple.md':
        '''
        # Примечания

        Примечания бывают разные.

        {% note %}

        Самые простые.

        {% endnote %}
        ''',
    'note_type.md': '''
        И посложнее.

        {% note warning %}

        Примечаниями часто злоупотребляют.

        Не стоит этого делать.

        - Причина 1.

        - Причина 2.

        {% endnote %}

        Однако, всё равно пусть работают.
        ''',
    'note_heading.md': '''
        {% note "JFYI" %}
        Примечание может иметь заголовок.
        {% endnote %}

        Или же

        {% note tip "" %}
        не иметь его вовсе.
        {% endnote %}
        ''',
    'table_inside.md': '''
        {% note tip %}

        Вот, например, таблица.

        First Header | Second Header
        ------------ | -------------
        Content from cell 1 | Content from cell 2
        Content in the first column | Content in the second column

        {% endnote %}
        ''',
    'nested_notes.md': '''
        {% note warning %}

        Будьте осторожны.

        {% note warning %}

        Это плохая идея.

        {% endnote %}

        {% endnote %}
        ''',
    'note_inside_list': '''
        Или так.

        - Раз.

          {% note warning %}

          В список Можно вложить список примечание.

          {% endnote %}

        - В список

          - Можно вложить список

            {% note tip %}

            А в него - примечание.

            - А в него - ещё список.

              - Раз.

              - Два.

            {% endnote %}
        ''',
    'note_inside_blockquote.md': '''
        Внутри blockquote примечания формально тоже могут быть

        > Только
        >
        > {% note alert %}
        >
        > Зачем?
        >
        > {% endnote %}
        >
        > Причём, blockquote тоже бывают вложенными:
        > >
        > > {% note alert %}
        > >
        > > Это уже *вообще* **перебор**.
        > >
        > > {% endnote %}
        ''',
    'russian_heading.md': '''
        ## Tale of sad Russian block

        {% note warning "Happy English block" %}
        Hi I am a happy bloke, everyone sees me!
        {% endnote %}

        {% note warning "Грустный русский блок" %}
        I am a sad block, nobody sees me :(
        {% endnote %}
    '''
}

TEST_CASES = dict((kv[0], textwrap.dedent(kv[1])) for kv in six.iteritems(TEST_CASES))


def _setup_renderers(jinja_templates):
    jinja = jinja2.Environment(
        loader=jinja2.DictLoader(jinja_templates),
        comment_start_string='{##',
        comment_end_string='##}',
    )
    jinja.add_extension('mkdocs_yandex.ext.jinja2.note.NoteTag')

    markdowner = markdown.Markdown(
        extensions=[
            'markdown.extensions.admonition',
            'markdown.extensions.tables',
            'mkdocs_yandex.ext.markdown.note',
        ],
        configs={},
    )

    run_context.jinja2_env = jinja

    return jinja, markdowner


@pytest.mark.parametrize('template_name', TEST_CASES.keys())
def test_note_extension(template_name):
    jinja, markdowner = _setup_renderers(TEST_CASES)

    md = jinja.get_template(template_name).render()

    html = markdowner.reset().convert(md)

    return html


def test_deprecated_note():
    jinja, markdowner = _setup_renderers({
        'deprecated_note.md':
            textwrap.dedent('''
            !!! info
                This will raise a syntax deprecation error
            '''),
    })

    md = jinja.get_template('deprecated_note.md').render()

    with pytest.raises(md_ext.DeprecatedSyntaxError) as e:
        markdowner.reset().convert(md)
    assert e.value.subject == 'admonition syntax for notes'
