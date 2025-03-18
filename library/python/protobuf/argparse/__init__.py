import argparse
import google.protobuf.json_format as json_format
import google.protobuf.text_format as text_format
import yaml
import json
from argparse import ArgumentError
import library.cpp.getoptpb.proto.confoption_pb2 as confoption_pb2

__all__ = ['ArgumentParser', 'ArgumentError']


def get_field_conf(field):
    opts = field.GetOptions()
    if opts.HasExtension(confoption_pb2.Conf):
        return opts.Extensions[confoption_pb2.Conf]
    return None


def get_name(field):
    return field.name

# TODO: enum options
# TODO: help titles
# TODO: subcommands


class SetValue(argparse.Action):

    def __init__(self, dest_path, option_strings, dest, **kws):
        self._value = kws.pop('set_value', None)
        self._dest_path = dest_path
        super(SetValue, self).__init__(option_strings, dest, **kws)

    def __call__(self, parser, namespace, values, option_string=None):
        dst = namespace._proto_result
        for x in self._dest_path[:-1]:
            dst = getattr(dst, x)
        field = dst.DESCRIPTOR.fields_by_name[self._dest_path[-1]]
        if field.label == field.LABEL_REPEATED:
            getattr(dst, self._dest_path[-1]).extend([json_format._ConvertScalarFieldValue(v, field) for v in values])
        else:
            if self._value is not None:
                setattr(dst, self._dest_path[-1], self._value)
            else:
                setattr(dst, self._dest_path[-1], json_format._ConvertScalarFieldValue(values, field))


class ParseProto(argparse.Action):

    def __init__(self, format, option_strings, dest, **kws):
        self._format = format
        super(ParseProto, self).__init__(option_strings, dest, **kws)

    def __call__(self, parser, namespace, values, option_string=None):
        dst = namespace._proto_result
        setattr(namespace, '_{}_config'.format(self._format), values)
        with open(values) as infile:
            if self._format == 'json':
                json_format.ParseDict(json.load(infile), dst)
            elif self._format == 'yaml':
                json_format.ParseDict(yaml.load(infile, Loader=yaml.FullLoader), dst)
            elif self._format == 'pb':
                text_format.Merge(infile.read(), dst)
            else:
                raise ValueError(self._format)


class ArgumentParser(argparse.ArgumentParser):

    def __init__(self, message_class, *args, **kws):
        self._json_config_option = kws.pop('config_json_option', '--config-json')
        self._yaml_config_option = kws.pop('config_yaml_option', '--config-yaml')
        self._pb_config_option = kws.pop('config_pb_option', '--config-pb')
        super(ArgumentParser, self).__init__(*args, **kws)
        self._message_class = message_class
        self._add_arguments(self._message_class.DESCRIPTOR)
        self.add_argument(self._json_config_option, help='path to config in json format', dest='_json_config', action=lambda *args, **kws: ParseProto('json', *args, **kws))
        self.add_argument(self._yaml_config_option, help='path to config in yaml format', dest='_yaml_config', action=lambda *args, **kws: ParseProto('yaml', *args, **kws))
        self.add_argument(self._pb_config_option, help='path to config in text protobuf format', dest='_pb_config', action=lambda *args, **kws: ParseProto('pb', *args, **kws))

    def _add_arguments(self, message_descriptor, prefix='', path=[]):
        for field in message_descriptor.fields:
            name = get_name(field)
            new_prefix = prefix + name
            new_path = list(path) + [name]

            conf = get_field_conf(field)
            if conf and conf.Ignore:
                continue

            if field.type == field.TYPE_MESSAGE:
                if field.label != field.LABEL_REPEATED:
                    self._add_arguments(field.message_type, prefix=new_prefix + '.', path=new_path)
            else:
                kws = {}
                if field.label == field.LABEL_REPEATED:
                    kws['nargs'] = '+'
                if conf and conf.Descr:
                    kws['help'] = conf.Descr
                option_name = new_prefix
                args = []
                if conf and conf.HasField('Long'):
                    option_name = conf.Long
                args.append('--' + option_name)
                if conf and conf.HasField('Short'):
                    args.append('-' + conf.Short)

                if field.type == field.TYPE_ENUM:
                    kws['choices'] = [v.name for v in field.enum_type.values]

                if field.type == field.TYPE_BOOL:
                    kws['set_value'] = True
                    self.add_argument(*args, nargs=0, action=lambda *args, **kws: SetValue(new_path, *args, **kws), **kws)
                    kws['set_value'] = False
                    self.add_argument('--no-' + option_name, nargs=0, action=lambda *args, **kws: SetValue(new_path, *args, **kws), **kws)
                else:
                    self.add_argument(*args, dest='.'.join(new_path), action=lambda *args, **kws: SetValue(new_path, *args, **kws), **kws)

    def parse_args(self, args=None, namespace=None):
        if namespace is None:
            namespace = argparse.Namespace()
        namespace._proto_result = self._message_class()
        args = super(ArgumentParser, self).parse_args(args=args, namespace=namespace)
        if not namespace._proto_result.IsInitialized():
            raise ValueError('initalization of arguments errors', namespace._proto_result.FindInitializationErrors())
        return args, namespace._proto_result
