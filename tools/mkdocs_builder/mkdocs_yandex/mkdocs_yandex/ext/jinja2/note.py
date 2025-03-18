from jinja2 import nodes
from jinja2.ext import Extension

from mkdocs_yandex.loggers import get_logger

log = get_logger(__name__)


class NoteTag(Extension):
    tags = {'note'}

    def parse(self, parser):
        line = next(parser.stream)
        lineno = line.lineno
        note_type = nodes.Const('info')
        note_title = nodes.Const(None)

        if parser.stream.skip_if('name:note'):
            note_type = nodes.Const('info')
        if parser.stream.skip_if('name:tip'):
            note_type = nodes.Const('tip')
        if parser.stream.skip_if('name:warning'):
            note_type = nodes.Const('warning')
        if parser.stream.skip_if('name:info'):
            note_type = nodes.Const('info')
        if parser.stream.skip_if('name:alert'):
            note_type = nodes.Const('error')

        if parser.stream.current.type == 'string':
            note_title = parser.parse_expression()

        body = parser.parse_statements(['name:endnote'], drop_needle=True)

        res = nodes.CallBlock(
            self.call_method('_render', [note_type, note_title], [], None, None),
            [], [], body,
        ).set_lineno(lineno)

        return res

    # We can generate any markdown/HTML at the preprocessing stage.
    def _render(self, note_type, note_title, caller=None):
        body = caller()
        lines = body.splitlines()
        # They use "" to suppress note header
        title_part = '' if note_title is None else ' "' + note_title + '"'

        new_lines = ['??? ' + note_type + title_part]

        for line in lines[1:]:
            if line.startswith('> '):
                for i, c in enumerate(line):
                    if not c.isspace() and c != '>':
                        break
                new_lines.append(line[:i] + '    ' + line[i:])
            else:
                if line.isspace() or line == '>':
                    new_lines.append(line)
                else:
                    new_lines.append('    ' + line)

        res = '\n'.join(new_lines).rstrip()

        return res
