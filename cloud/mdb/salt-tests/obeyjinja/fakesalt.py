from jinja2 import nodes
from jinja2.ext import Extension


class SerializerExtension(Extension, object):

    tags = set(['load_yaml', 'load_json', 'import_yaml', 'import_json', 'load_text', 'import_text'])

    def __init__(self, environment):
        super(SerializerExtension, self).__init__(environment)

    _load_parsers = set(['load_yaml', 'load_json', 'load_text'])

    def parse(self, parser):
        if parser.stream.current.value == 'import_yaml':
            return self.parse_yaml(parser)
        elif parser.stream.current.value == 'import_json':
            return self.parse_json(parser)
        elif parser.stream.current.value == 'import_text':
            return self.parse_text(parser)
        elif parser.stream.current.value in self._load_parsers:
            return self.parse_load(parser)

        parser.fail('Unknown format ' + parser.stream.current.value, parser.stream.current.lineno)

    # pylint: disable=E1120,E1121
    def parse_load(self, parser):
        next(parser.stream).lineno
        parser.stream.expect('name:as')
        parser.parse_assign_target()
        parser.free_identifier()
        parser.parse_statements(('name:endload',), drop_needle=True)
        return nodes.TemplateData('fake')

    def parse_yaml(self, parser):
        parser.parse_import()
        return nodes.TemplateData('fake')

    def parse_json(self, parser):
        parser.parse_import()
        return nodes.TemplateData('fake')

    def parse_text(self, parser):
        parser.parse_import()
        return nodes.TemplateData('fake')
