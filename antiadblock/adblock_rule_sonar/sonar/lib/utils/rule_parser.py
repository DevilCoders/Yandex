# coding=utf-8
"""Parser for All adblocks syntaxes"""

import re
from collections import namedtuple


class ParseError(Exception):
    """Exception thrown by the parser when it encounters invalid input.

    Parameters
    ----------
    error : str
        Description of the error.
    text : str
        The source text that caused an error.

    """

    def __init__(self, error, text):
        Exception.__init__(self, '{} in "{}"'.format(error, text))
        self.text = text
        self.error = error


class SelectorType:
    """Selector type constants."""
    # url selector
    URL_PATTERN = 'url-pattern'        # Normal URL patterns.
    URL_REGEXP = 'url-regexp'          # Regular expressions for URLs.

    # element selector
    CSS = 'css'                        # CSS selectors for hiding filters.
    XCSS = 'extended-css'              # Extended CSS selectors (to emulate CSS4).

    # any internal scripts
    SNIPPET = 'snippet'                # Adblock or Ublock internal snippet.
    ADGUARD_SCRIPT = 'adguard-script'  # Adguard scripts, like 'var AG_onLoad=function(...)'.


class FilterAction:
    """Filter action constants."""

    BLOCK = 'block'              # Block the request.
    ALLOW = 'allow'              # Allow the request (whitelist).
    HIDE = 'hide'                # Hide selected element(s).
    SHOW = 'show'                # Show selected element(s) (whitelist).


class FilterOption:
    """Filter option constants."""

    # Resource types.
    OTHER = 'other'
    SCRIPT = 'script'
    IMAGE = 'image'
    STYLESHEET = 'stylesheet'
    OBJECT = 'object'
    SUBDOCUMENT = 'subdocument'
    DOCUMENT = 'document'
    WEBSOCKET = 'websocket'
    WEBRTC = 'webrtc'
    PING = 'ping'
    XMLHTTPREQUEST = 'xmlhttprequest'
    OBJECT_SUBREQUEST = 'object-subrequest'
    MEDIA = 'media'
    FONT = 'font'
    MP4 = 'mp4'
    COOKIE = 'cookie'
    POPUP = 'popup'
    GENERICBLOCK = 'genericblock'
    GENERICHIDE = 'generichide'

    # Deprecated resource types.
    BACKGROUND = 'background'
    XBL = 'xbl'
    DTD = 'dtd'

    # Other options.
    MATCH_CASE = 'match-case'
    DOMAIN = 'domain'
    THIRD_PARTY = 'third-party'
    COLLAPSE = 'collapse'
    SITEKEY = 'sitekey'
    DONOTTRACK = 'donottrack'
    CSP = 'csp'
    REWRITE = 'rewrite'
    REPLACE = 'replace'
    IMPORTANT = 'important'
    EMPTY = 'empty'

    # Exception modifiers in Adguard.
    ELEMHIDE = 'elemhide'
    JSINJECT = 'jsinject'
    CONTENT = 'content'
    URLBLOCK = 'urlblock'
    EXTENSION = 'extension'
    BADFILTER = 'badfilter'
    STEALTH = 'stealth'


def _line_type(name, field_names, format_string):
    """Define a line type.

    Parameters
    ----------
    name: str
        The name of the line type to define.
    field_names: str or list
        A sequence of field names or one space-separated string that contains
        all field names.
    format_string: str
        A format specifier for converting this line type back to string
        representation.

    Returns
    -------
    class
        Class created with `namedtuple` that has `.type` set to lowercased
        `name` and supports conversion back to string with `.to_string()`
        method.

    """
    lt = namedtuple(name, field_names)
    lt.type = name.lower()
    lt.to_string = lambda self: format_string.format(self)
    return lt


Header = _line_type('Header', 'version', '[{.version}]')
EmptyLine = _line_type('EmptyLine', '', '')
Comment = _line_type('Comment', 'text', '! {.text}')
Metadata = _line_type('Metadata', 'key value', '! {0.key}: {0.value}')
Filter = _line_type('Filter', 'text selector action options domains', '{.text}')
Include = _line_type('Include', 'target', '%include {0.target}%')


METADATA_REGEXP = re.compile(r'\s*!\s*(.*?)\s*:\s*(.*)')
INCLUDE_REGEXP = re.compile(r'%include\s+(.+)%')
HEADER_REGEXP = re.compile(r'\[(Adblock(?:\s*Plus\s*[\d\.]+?)?)\]', flags=re.I)
HIDING_FILTER_REGEXP = re.compile(r'^(?P<domain>[^/|@\"!]*?)(?P<type_flag>#[@?%$]{0,3}#|\$@?\$)(?P<rule_text>.+)$')
FILTER_OPTIONS_REGEXP = re.compile(r'(?<![\\$@])\$(~?[a-z][\w-]+(?:=(.+?))?(?:(?<!\\),~?[a-z][\w-]+(?:=.+?)?)*)$')


def _parse_instruction(text):
    match = INCLUDE_REGEXP.match(text)
    if not match:
        raise ParseError('Unrecognized instruction', text)
    return Include(match.group(1))


def _parse_option(option):
    if '=' in option:
        return option.split('=', 1)
    if option.startswith('~'):
        return option[1:], False
    return option, True


def _parse_filter_option(option):
    name, value = _parse_option(option)

    # Handle special cases of multivalued options.
    if name == FilterOption.DOMAIN:
        value = [_parse_option(o) for o in value.split('|')]
    elif name == FilterOption.SITEKEY:
        value = value.split('|')

    return name, value


def _parse_filter_options(options):
    return [_parse_filter_option(o) for o in re.split(r'(?<!\\),', options)]


def _parse_blocking_filter(text):
    action = FilterAction.BLOCK
    options = []
    selector = text

    if selector.startswith('@@'):
        action = FilterAction.ALLOW
        selector = selector[2:]

    if '$' in selector:
        opt_match = FILTER_OPTIONS_REGEXP.search(selector)
        if opt_match:
            selector = selector[:opt_match.start(0)]
            options = _parse_filter_options(opt_match.group(1))

    if (len(selector) > 1
            and selector.startswith('/') and selector.endswith('/')):
        selector = {'type': SelectorType.URL_REGEXP, 'value': selector[1:-1]}
    else:
        selector = {'type': SelectorType.URL_PATTERN, 'value': selector}

    domains = sum([opt[1] for opt in options if opt[0] == FilterOption.DOMAIN], [])
    options = [opt for opt in options if opt[0] != FilterOption.DOMAIN]

    return Filter(text, selector, action, options, domains)


def _parse_hiding_filter(text, domain, type_flag, selector_value):
    """
    :param text: полный текст правила
    :param domain: домены которые удалось выпарсить из правила
    :param type_flag: разделитель в правиле (определяет что это за тип правила)
    :param selector_value: селектор на элемент на котором применять правило, или текст скрипта правила
    :return: объект фильтра
    """
    selector = {'type': SelectorType.CSS, 'value': selector_value}
    action = FilterAction.HIDE
    options = []

    # Adguard-о специфичные правила. Вырезают(а не скрывают или блокируют) из дом-дерева скрипт/элемент/все что угодно
    if type_flag[0] == type_flag[-1] == '$':
        selector['type'] = SelectorType.SNIPPET

    type_flag = type_flag[1:-1]
    if type_flag.startswith('@'):
        action = FilterAction.SHOW
        type_flag = type_flag[1:]
    if type_flag == '?':
        selector['type'] = SelectorType.XCSS
    elif type_flag == '%':
        selector['type'] = SelectorType.ADGUARD_SCRIPT
    # в ublock почему-то скрипты `+js(` находятся за ## и иначе определяются как hide-element правила
    elif type_flag == '$' or selector_value.startswith('+js('):
        selector['type'] = SelectorType.SNIPPET

    domains = [_parse_option(d) for d in domain.split(',')] if domain else []

    return Filter(text, selector, action, options, domains)


def parse_filter(text):
    """
    Parse one filter.
    :param text: str Filter to parse in ABP filter list syntax.
    :return: namedtuple Parsed filter.
    """
    if re.search(r'#|\$@?\$', text):
        match = HIDING_FILTER_REGEXP.search(text)
        if match:
            return _parse_hiding_filter(text, *match.groups())
    return _parse_blocking_filter(text)


def parse_line(line, position='body'):
    """Parse one line of a filter list.

    The types of lines that that the parser recognizes depend on the position.
    If position="body", the parser only recognizes filters, comments,
    processing instructions and empty lines. If position="metadata", it in
    addition recognizes metadata. If position="start", it also recognizes
    headers.

    Note: Checksum metadata lines are recognized in all positions for backwards
    compatibility. Historically, checksums can occur at the bottom of the
    filter list. They are are no longer used by Adblock Plus, but in order to
    strip them (in abp.filters.renderer), we have to make sure to still parse
    them regardless of their position in the filter list.

    Parameters
    ----------
    line : str
        Line of a filter list.
    position : str
        Position in the filter list, one of "start", "metadata" or "body"
        (default is "body").

    Returns
    -------
    namedtuple
        Parsed line (see `_line_type`).

    Raises
    ------
    ParseError
        ParseError: If the line can't be parsed.

    """
    positions = {'body', 'start', 'metadata'}
    if position not in positions:
        raise ValueError('position should be one of {}'.format(positions))

    if isinstance(line, type(b'')):
        line = line.decode('utf-8')

    stripped = line.strip()

    if stripped == '':
        return EmptyLine()

    if position == 'start':
        match = HEADER_REGEXP.search(line)
        if match:
            return Header(match.group(1))

    if stripped.startswith('!'):
        match = METADATA_REGEXP.match(line)
        if match:
            key, value = match.groups()
            if position != 'body' or key.lower() == 'checksum':
                return Metadata(key, value)
        return Comment(stripped[1:].lstrip())

    if stripped.startswith('%include') and stripped.endswith('%'):
        return _parse_instruction(stripped)

    return parse_filter(stripped)


def parse_filterlist(lines):
    """Parse filter list from an iterable.

    Parameters
    ----------
    lines: iterable of str
        Lines of the filter list.

    Returns
    -------
    iterator of namedtuple
        Parsed lines of the filter list.

    Raises
    ------
    ParseError
        Thrown during iteration for invalid filter list lines.
    TypeError
        If `lines` is not iterable.

    """
    position = 'start'

    for line in lines:
        parsed_line = parse_line(line, position)
        yield parsed_line

        if position != 'body' and parsed_line.type in {'header', 'metadata'}:
            # Continue parsing metadata until it's over...
            position = 'metadata'
        else:
            # ...then switch to parsing the body.
            position = 'body'
