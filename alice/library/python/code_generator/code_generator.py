# coding: utf-8

from __future__ import division, unicode_literals

import json
import re

from contextlib import contextmanager


BANNER_LENGTH = 70
CAMELIZE_PATTERN = re.compile('_([0-9a-zA-Z])')
PASCALIZE_PATTERN = re.compile('(^|_)([0-9a-zA-Z])')


class CodeGenerator(object):
    def __init__(self, fobj):
        self._fobj = fobj
        self._indent_level = 0
        self._prev_line_empty = True

    @contextmanager
    def indent(self):
        self._indent_level += 4
        try:
            yield
        finally:
            self._indent_level -= 4
            assert self._indent_level >= 0

    @contextmanager
    def disable_warning(self, warning):
        self.write_line('#pragma clang diagnostic push', indent_level=0)
        self.write_line('#pragma clang diagnostic ignored "{}"'.format(warning), indent_level=0)
        try:
            yield
        finally:
            self.write_line('#pragma clang diagnostic pop', indent_level=0)

    def write_line(self, line, indent_level=None):
        assert line, 'write_line must not be used for printing empty lines, use skip_line for that'

        if indent_level is None:
            indent_level = self._indent_level

        padded_line = indent_level * ' ' + line + '\n'
        self._fobj.write(padded_line.encode('utf-8'))
        self._prev_line_empty = False

    def skip_line(self):
        """Prints an empty line. Subsequent empty lines are fused into one.
        """
        if not self._prev_line_empty:
            self._fobj.write(b'\n')
            self._prev_line_empty = True

    def banner(self, generator, comment_sequence=None, source_path=None):
        if comment_sequence is None:
            comment_sequence = self.comment_sequence

        self.write_line(BANNER_LENGTH // len(comment_sequence) * comment_sequence)
        self.write_line(comment_sequence + ' WARNING: this code was automatically generated by {},'.format(generator))
        self.write_line(comment_sequence + ' don\'t edit it manually.')
        if source_path is not None:
            self.write_line(comment_sequence)
            self.write_line(comment_sequence + ' Source path: ' + source_path)
        self.write_line(BANNER_LENGTH // len(comment_sequence) * comment_sequence)
        self.skip_line()


class CppCodeGenerator(CodeGenerator):
    comment_sequence = '//'

    def __init__(self, fobj):
        super(CppCodeGenerator, self).__init__(fobj)
        self.in_enum = False

    @contextmanager
    def namespace(self, name):
        self.write_line('namespace {} {{'.format(name))
        self.skip_line()
        try:
            yield
        finally:
            self.skip_line()
            self.write_line('}}  // namespace {}'.format(name))

    @contextmanager
    def class_def(self, decl):
        self.write_line(decl + ' {')
        try:
            with self.indent():
                yield
        finally:
            self.write_line('};')
            self.skip_line()

    @contextmanager
    def func_def(self, decl):
        self.write_line(decl + ' {')
        try:
            with self.indent():
                yield
        finally:
            self.write_line('}')
            self.skip_line()

    @contextmanager
    def enum_class(self, name):
        self.write_line('enum class {} {{'.format(name))
        try:
            self.in_enum = True
            with self.indent():
                yield
        finally:
            self.in_enum = False
            self.write_line('};')
            self.skip_line()

    def enum_option(self, name, value, serialization=None):
        assert self.in_enum

        out = '{} = {}'.format(name, value)
        if serialization is not None:
            out += ' /* "{}" */'.format(serialization)
        out += ','
        self.write_line(out)

    def pragma(self, pragma):
        self.write_line('#pragma {}'.format(pragma))

    def include(self, path, system=False):
        self.write_line('#include {}{}{}'.format(
            '<' if system else '"',
            path,
            '>' if system else '"',
        ))


class ProtobufCodeGenerator(CodeGenerator):
    comment_sequence = '//'

    def __init__(self, fobj):
        super(ProtobufCodeGenerator, self).__init__(fobj)
        self._proto3 = False
        self._inside_message = False
        self._inside_enum = False
        self._inside_oneof = False

    def syntax3(self):
        self._proto3 = True
        self.write_line('syntax = "proto3";')
        self.skip_line()

    def imports(self, *imports):
        for import_path in imports:
            self.write_line('import "{}";'.format(import_path))
        self.skip_line()

    def package(self, name):
        self.write_line('package {};'.format(name))
        self.skip_line()

    @contextmanager
    def message(self, name):
        self.write_line('message {} {{'.format(name))
        self._inside_message = True
        try:
            with self.indent():
                yield
        finally:
            self._inside_message = False
            self.write_line('}')
            self.skip_line()

    def field(self, field_type, field_name, field_num, repeated=False, required=False, extra=None, **kwargs):
        assert self._inside_message or self._inside_oneof

        if self._proto3:
            qualifier = ''
        else:
            qualifier = 'required ' if required else 'optional '

        if repeated:
            assert not self._inside_oneof
            qualifier = 'repeated '

        self.write_line('{qualifier}{field_type} {field_name} = {field_num}{options};'.format(
            qualifier='' if self._inside_oneof or field_type.startswith('map<') else qualifier,
            field_type=field_type,
            field_name=field_name,
            field_num=field_num,
            options=_make_pb_options(extra, kwargs),
        ))

    @contextmanager
    def enum(self, name):
        self.write_line('enum {} {{'.format(name))
        self._inside_enum = True
        try:
            with self.indent():
                yield
        finally:
            self._inside_enum = False
            self.write_line('}')
            self.skip_line()

    def enum_option(self, name, value, extra=None, **kwargs):
        self.write_line('{name} = {value}{options};'.format(
            name=name,
            value=value,
            options=_make_pb_options(extra, kwargs),
        ))

    @contextmanager
    def oneof(self, name):
        self.write_line('oneof {} {{'.format(name))
        self._inside_oneof = True
        try:
            with self.indent():
                yield
        finally:
            self._inside_oneof = False
            self.write_line('}')


class YaMakeCodeGenerator(CodeGenerator):
    comment_sequence = '#'

    def __init__(self, fobj):
        super(YaMakeCodeGenerator, self).__init__(fobj)

    @contextmanager
    def target(self, kind, name=''):
        self.write_line('{}({})'.format(kind, name))
        self.skip_line()
        try:
            yield
        finally:
            self.write_line('END()')

    def section(self, kind, items):
        self.write_line('{}('.format(kind))
        with self.indent():
            for item in items:
                self.write_line(item)
        self.write_line(')')
        self.skip_line()


def pascalize(name):
    return PASCALIZE_PATTERN.sub(lambda match: match.group(2).upper(), name)


def camelize(name):
    return CAMELIZE_PATTERN.sub(lambda match: match.group(1).upper(), name)


def stringbuf(value):
    if isinstance(value, bytes):
        value = value.decode('utf-8')

    result = json.dumps(value, ensure_ascii=False)
    if isinstance(result, bytes):
        result = result.decode('utf-8')

    return 'TStringBuf({})'.format(result)


def _make_pb_options(extra, kwargs):
    if extra is None:
        extra = {}
    if kwargs:
        extra.update(kwargs)

    if extra:
        return ' [{}]'.format(', '.join(
            '{} = {}'.format(key, json.dumps(value, ensure_ascii=False))
            for key, value in kwargs.items()
        ))
    else:
        return ''
