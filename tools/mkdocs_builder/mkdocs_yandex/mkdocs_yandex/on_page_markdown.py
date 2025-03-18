import os

import bs4

from mkdocs_yandex import run_context
from mkdocs_yandex.ext import markdown as md_ext
from mkdocs_yandex.ext.markdown import indent, full_strict
from mkdocs_yandex.loggers import get_logger

logger = get_logger(__name__)


def do(markdown, page, config, files, plugin_config):
    if plugin_config['full_strict']:
        config['markdown_extensions'].append(full_strict.FullStrictExtension(page.file, files))

    file_rel_path = page.file.src_path
    file_abs_path = os.path.join(config['docs_dir'], file_rel_path)
    # file_abs_dir = os.path.dirname(file_abs_path)

    # Context
    run_context.page_src_path = file_rel_path
    run_context.page_url = page.url
    run_context.page_file_abs_path = file_abs_path
    run_context.file_being_processed = file_abs_path

    # Jinja2 preprocessing.
    # Conditional logic, variables resolution etc
    jinja2_template = run_context.jinja2_env.from_string(markdown)
    preprocessed_page = jinja2_template.render(run_context.config.get('extra', {}))

    sanitize_html(preprocessed_page)

    return preprocessed_page


def sanitize_html(md):

    def tab(text):
        return ' ' * indent.TAB_LENGTH + text

    def strip(text):
        if text.startswith('\n'):
            text = text[1:]
        if len(text) > 0 and text[-1] == '\n':
            text = text[:-1]
        return text

    md_without_code_sections = filter_code_sections(md)

    bs = bs4.BeautifulSoup(md_without_code_sections, 'lxml')
    details = bs.find_all('details')
    if details:
        details = details[0]
        summary = details.find('summary', recursive=False)
        text = []

        if summary:
            for c in details.contents[details.index(summary) + 1:]:
                text.extend([tab(l) for l in strip(c).splitlines()])
        text = '\n'.join(text)
        raise md_ext.DeprecatedSyntaxError(
            '<details> html tag', details,
            '{{% list details %}}\n'
            '\n'
            '- {summary}\n'
            '\n'
            '{text}\n'
            '\n'
            '{{% endlist %}}'
            .format(summary=summary.text or '`Put your summary here`', text=text or details.text)
        )

    a = bs.find_all(lambda tag: tag.name == "a" and 'name' in tag.attrs)
    if a:
        a = a[0]
        raise md_ext.DeprecatedSyntaxError(subject='<a name="..."> html tag', where=a, replacement='{{ #{id} }}'.format(id=a['name']))


# Source markdown can contain code sections, for example:
#
# `<Details />` - here goes some description of the component.
#
# or
#
# ```js
# // ... inside your react component
# render() {
#     return <Details
#         title='Some title'
#     />
# }
# ```
#
# when BeautifulSoup meets tag <details> in the markdown, it doesn't see that this tag is embraced by 1- or 3-backticks
# and throws an exception that deprecated tag of markdown is used
#
# to avoid this case, the function "filter_code_sections" removes all code sections from source markdown,
# then you can safely sanitize it

def filter_code_sections(md):
    REMOVE_CHAR = True

    BT = '`'

    # W = WAITING
    # BT = BACKTICK

    W_OPENING_BT = 'WAITING_OPENING_BACKTICK'
    W_2ND_BT_OR_NON_BT = 'WAITING_2ND_BACKTICK_OR_NON_BACKTICK'
    W_3RD_BT = 'WAITING_3RD_BACKTICK'
    W_NON_BT_BEFORE_CLOSING_BT = 'WAITING_NON_BACKTICK_BEFORE_CLOSING_BACKTICK'
    W_CLOSING_1ST_BT_OR_NON_BT = 'WAITING_CLOSING_1ST_BACKTICK_OR_NON_BACKTICK'
    W_CLOSING_BT = 'WAITING_CLOSING_BACKTICK'
    W_NON_BT = 'WAITING_NON_BACKTICK'
    W_CLOSING_2ND_BT = 'WAITING_CLOSING_2ND_BACKTICK'
    W_CLOSING_3RD_BT = 'WAITING_CLOSING_3RD_BACKTICK'
    FAILED = 'FAILED'

    # see state-chart of finite state machine here:
    # https://jing.yandex-team.ru/files/rashid/filter-code-sections-state-chart.png

    state_handlers = {
        W_OPENING_BT: lambda c: (W_2ND_BT_OR_NON_BT, REMOVE_CHAR) if c == BT else (W_OPENING_BT, not REMOVE_CHAR),
        W_2ND_BT_OR_NON_BT: lambda c: (W_3RD_BT, REMOVE_CHAR) if c == BT else (W_CLOSING_BT, REMOVE_CHAR),
        W_3RD_BT: lambda c: (W_NON_BT_BEFORE_CLOSING_BT, REMOVE_CHAR) if c == BT else (FAILED, REMOVE_CHAR),
        W_NON_BT_BEFORE_CLOSING_BT: lambda c: (W_CLOSING_1ST_BT_OR_NON_BT, REMOVE_CHAR) if c != BT else (FAILED, REMOVE_CHAR),
        W_CLOSING_BT: lambda c: (W_NON_BT, REMOVE_CHAR) if c == BT else (W_CLOSING_BT, REMOVE_CHAR),
        W_NON_BT: lambda c: (W_OPENING_BT, not REMOVE_CHAR) if c != BT else (FAILED, REMOVE_CHAR),
        W_CLOSING_1ST_BT_OR_NON_BT: lambda c: (W_CLOSING_2ND_BT, REMOVE_CHAR) if c == BT else (W_CLOSING_1ST_BT_OR_NON_BT, REMOVE_CHAR),
        W_CLOSING_2ND_BT: lambda c: (W_CLOSING_3RD_BT, REMOVE_CHAR) if c == BT else (W_CLOSING_1ST_BT_OR_NON_BT, REMOVE_CHAR),
        W_CLOSING_3RD_BT: lambda c: (W_NON_BT, REMOVE_CHAR) if c == BT else (W_CLOSING_1ST_BT_OR_NON_BT, REMOVE_CHAR),
    }

    state = W_OPENING_BT
    md_without_code_sections = ''

    for c in md:
        state, remove_char = state_handlers[state](c)

        if state == FAILED:
            return md

        if remove_char:
            continue

        md_without_code_sections = md_without_code_sections + c

    return md_without_code_sections
