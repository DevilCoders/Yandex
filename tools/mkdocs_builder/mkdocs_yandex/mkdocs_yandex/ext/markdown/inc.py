import codecs
import os
import re

import xml.etree.ElementTree as etree

from markdown import Extension
from markdown.blockprocessors import BlockProcessor
from markdown.extensions.codehilite import CodeHilite
from markdown.treeprocessors import Treeprocessor

from six.moves.urllib.parse import urlparse, urlunparse

from mkdocs_yandex import run_context
from mkdocs_yandex.loggers import get_logger
from mkdocs_yandex.regex import RE_INC, RE_CODE
from mkdocs_yandex.regex import RE_MD_LINK
from mkdocs_yandex.util import is_int_positive
from mkdocs_yandex.util import get_params


def makeExtension(**kwargs):
    return IncExtension(**kwargs)


class IncBlockProcessor(BlockProcessor):
    log = get_logger(__name__)

    RE_INC = re.compile(RE_INC, re.UNICODE)
    RE_CODE = re.compile(RE_CODE, re.UNICODE)
    RE_MD_LINK = re.compile(RE_MD_LINK, re.UNICODE)
    END_MARKER = 'MKDOCS_PLUGIN_MARKDOWN_YANDEX_MARKDOWN_EXTENSION_INC_BLOCK_PROCESSOR_END_MARKER'

    MD_INCLUDES_DIRECTORY = '_includes'

    def __init__(self, parser):
        self.md = parser.md
        super(IncBlockProcessor, self).__init__(parser)

    def test(self, parent, block):
        return bool(self.RE_INC.search(block)) or bool(self.RE_CODE.search(block))

    def run(self, parent, blocks):

        block = blocks.pop(0)

        # TODO:
        # Should we process multiple incs within single line (and keep content between them)?
        # At the moment, I think not.

        m = self.RE_INC.match(block)
        if m:
            inc_type = m.group('type')
        else:
            m = self.RE_CODE.match(block)
            inc_type = 'literal'

        if not m:
            self.log.error(
                "Something went wrong, can't parse `include` or `code` statement. {} contact devtools@ and share your local diff.".format(
                    "Try insert blank line after `{}`. If that doesn't work, please".format(block.split("\n")[0]) if "\n" in block else "Please"
                )
            )
            raise SystemExit(1)

        current_file = run_context.file_being_processed
        current_dir = os.path.dirname(current_file)
        inc_path = m.group('ref')

        md_link = self.RE_MD_LINK.match(inc_path)
        if md_link:
            inc_path = md_link.group('ref')

        if os.path.isabs(inc_path):
            inc_file = os.path.normpath(os.path.join(run_context.config['docs_dir'], inc_path[1:]))
        else:
            inc_file = os.path.normpath(os.path.join(current_dir, inc_path))

        opts = m.group('opts')
        self.params = get_params(opts) if opts else dict()
        unindent = False if 'keep_indents' in m.groupdict() else True

        if not inc_file.startswith(run_context.config['docs_dir']):
            self.log.warn(
                'Included file "%s" is outside of the docs dir "%s" in "%s"',
                inc_file, run_context.config['docs_dir'], run_context.file_being_processed,
            )
            return

        if not inc_type:
            if os.path.basename(os.path.dirname(inc_file)) != self.MD_INCLUDES_DIRECTORY:
                self.log.warn(
                    'Included file "%s" must lay directly in "%s" dir with name (in "%s")',
                    inc_file, self.MD_INCLUDES_DIRECTORY, run_context.file_being_processed,
                )

        if not os.path.isfile(inc_file):
            self.log.warn(
                'Included file "%s" resolved to "%s" does not exist (in "%s")',
                inc_path, inc_file, run_context.file_being_processed,
            )
            return

        with codecs.open(inc_file, 'r', encoding='utf-8') as f:
            lines, not_found_lines, not_found_markers = self.filter_lines(f.read().split('\n'))
            lines = self.remove_code_indents(lines, unindent)
            if lines:
                lines[0] = m.group('before') + lines[0]
                lines[-1] += m.group('after')
            else:
                lines = [m.group('before') + m.group('after')]

                self.log.warn(
                    'Failed to include lines from %s to %s. Lines not found: %s. Markers not found: %s.',
                    os.path.relpath(inc_file, run_context.config['docs_dir']),
                    os.path.relpath(current_file, run_context.config['docs_dir']),
                    not_found_lines,
                    not_found_markers
                )

        if not inc_type:
            run_context.file_being_processed = inc_file
            container = etree.SubElement(parent, 'span')
            container.set('data-from', inc_file)

            # see markdown/core.py
            for prep in self.parser.md.preprocessors:
                lines = prep.run(lines)
            content = '\n'.join(lines)

            # --
            jinja2_template = run_context.jinja2_env.from_string(content)
            content = jinja2_template.render()

            self.parser.parseChunk(container, content)
            run_context.file_being_processed = current_file

        elif inc_type == 'literal':
            content = '\n'.join(lines)
            highlighter = CodeHilite(content, guess_lang=True)
            if 'lang' in self.params:
                highlighter.lang = self.params['lang']
            else:
                highlighter.guess_lang = True
            code = highlighter.hilite()
            placeholder = self.md.htmlStash.store(code)
            container = etree.SubElement(parent, 'span')
            container.text = placeholder

    def get_filter_lines_and_markers(self, total):
        """
        Based on parameters of `{% inc ... %}` instruction, produce a list of line indexes and a map of start-end markers
        :param total: count of lines in the file being included
        :return:
            linenums - list of line indexes (0-based) to be included
            markers  - a dict of markers, where key is a start marker and value is an end marker
            (all lines between them inclusively supposed to be included)
        :rtype: tuple
        """
        linenums = list()
        markers = dict()

        if 'lines' in self.params:
            parts = self.params['lines'].split(',')
            for part in parts:
                begend = part.strip().split('-')

                if ['', ''] == begend or len(begend) > 2:
                    self.log.warn('Invalid range "%s" in %s' % (part, run_context.file_being_processed))

                # line numbers
                if is_int_positive(begend[0]):
                    if len(begend) == 1:
                        linenums.append(self._num_to_index(begend[0]))
                    else:
                        if not is_int_positive(begend[1]) and begend[1]:
                            self.log.warn('Invalid line number "%s" in %s'
                                          % (begend[1], run_context.file_being_processed))

                        start = self._num_to_index(begend[0])  # left half open (cf. -10)
                        end = self._num_to_index(begend[1]) or max(start, total) - 1  # right half open (cf. 10-)
                        if start > end:  # invalid range (cf. 10-1)
                            self.log.warn('Invalid range "%s" in %s' % (part, run_context.file_being_processed))
                        linenums.extend(range(start, end + 1))
                # markers
                else:
                    if len(begend) == 1:
                        self.log.warn('Invalid marker range "%s" in "%s" - ignoring', part, run_context.file_being_processed)
                        markers[begend[0]] = begend[0]
                    else:
                        # if someone even happens to use this very word in a document - well, (: happens
                        markers[begend[0]] = begend[1] or self.END_MARKER

        return linenums, markers

    def _num_to_index(self, str_value):
        return int(str_value or 1) - 1

    def remove_code_indents(self, lines, unindent=False):
        if not unindent:
            return lines

        symbol = None
        symbols_to_remove = -1

        for line in lines:
            if line == '':
                continue

            if symbol is None and line[0] in [u' ', u'\t']:
                symbol = line[0]

            symbols_to_remove_in_line = 0

            for c in line:
                if c != symbol:
                    break
                else:
                    symbols_to_remove_in_line += 1

            if symbols_to_remove == -1:
                symbols_to_remove = symbols_to_remove_in_line
            else:
                symbols_to_remove = min(symbols_to_remove_in_line, symbols_to_remove)

        if symbols_to_remove == -1:
            symbols_to_remove = 0

        res = []

        for line in lines:
            res.append(line[symbols_to_remove:])

        return res

    def filter_lines(self, lines):
        linenums, markers = self.get_filter_lines_and_markers(len(lines))
        # Any opening marker enables recording mode and any closing marker disables it.
        # If desired, this can be changed later.
        in_recording_mode = False
        if not (linenums or markers):
            return lines, None, None
        newlines = list()
        found_lines = set()
        used_opening_markers = set()
        used_closing_markers = set([self.END_MARKER])

        for i, line in enumerate(lines):
            if i in linenums:
                newlines.append(line)
                found_lines.add(i)

            # lines with markers themselves are not included intentionally
            opening_markers_in_line = [marker for marker in markers if marker in line]
            closing_markers_in_line = [markers[o] for o in markers if markers[o] in line]
            if not in_recording_mode:
                if opening_markers_in_line:
                    in_recording_mode = True
                    used_opening_markers.update(opening_markers_in_line)
            elif closing_markers_in_line:
                in_recording_mode = False
                used_closing_markers.update(closing_markers_in_line)
            else:
                newlines.append(line)

        not_found_lines = sorted(list(set(linenums) - found_lines))
        not_found_markers = sorted(list(
            (set(markers) - used_opening_markers) |
            (set(markers.values()) - used_closing_markers)
        ))

        if not_found_lines or not_found_markers:
            newlines = list()

        return newlines, not_found_lines, not_found_markers


class IncTreeProcessor(Treeprocessor):
    def traverse(self, root):
        for el in root.iter():
            from_file = self.pop_include_source(el)
            if from_file is None:
                continue

            for subel in el.iter():
                self.pop_include_source(subel)

                if subel.tag == 'a':
                    self.fix_link(subel, 'href', from_file)
                elif subel.tag == 'img':
                    self.fix_link(subel, 'src', from_file)

    @staticmethod
    def pop_include_source(element):
        if element.tag == 'span' and 'data-from' in element.attrib:
            r = element.get('data-from')
            del element.attrib['data-from']
            return r

    @classmethod
    def fix_link(cls, element, attr_name, from_file):
        ref = element.get(attr_name)
        new_ref = cls.get_new_ref(from_file, ref)
        element.set(attr_name, new_ref)

    @staticmethod
    def get_new_ref(from_file, ref):
        dir_from = os.path.dirname(from_file)
        scheme, netloc, path, params, query, fragment = urlparse(ref)
        if not (scheme or netloc or os.path.isabs(ref)) and path:
            abs_path_to = os.path.join(dir_from, ref)
            rel_path_to = os.path.relpath(abs_path_to, os.path.dirname(run_context.page_file_abs_path))
            components = (scheme, netloc, rel_path_to, params, query, fragment)
            ref_new = urlunparse(components)
            return ref_new
        return ref

    def run(self, doc):
        self.traverse(doc)


class IncExtension(Extension):

    def extendMarkdown(self, md):
        inc_blockprocessor = IncBlockProcessor(md.parser)
        #                                              right after the paragraph processor
        md.parser.blockprocessors.register(inc_blockprocessor, 'inc_blockprocessor', 10.01)

        inc_treeprocessor = IncTreeProcessor(md)
        if "relpath" in md.treeprocessors:
            i = md.treeprocessors.get_index_for_name("relpath")
            after = md.treeprocessors._priority[i].priority
            if i > 0:
                before = md.treeprocessors._priority[i - 1].priority
            else:
                # Location is first item`
                before = after + 10
            priority = before - ((before - after) / 2)
        else:
            md.treeprocessors._sort()
            priority = md.treeprocessors._priority[-1].priority - 5
        md.treeprocessors.register(inc_treeprocessor, "inc_treeprocessor", priority)
