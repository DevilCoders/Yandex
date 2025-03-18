from library.python.protobuf.argparse import ArgumentParser
from library.python.protobuf.argparse.ut.test_pb2 import TArgs
import sys
import pytest
import json
import yaml


def test_argparse():
    parser = ArgumentParser(TArgs)
    args, proto = parser.parse_args(args=[
        '--s', 'hgg',
        '--i32', '-12',
        '--ui32', '34',
        '--ui64', '0',
        '--msg.x', 'foo',
        '--d', '3.14',
        '--arr', '1', '2',
        '--req', '123',
        '--long-named-option', 'loong',
        '--no-flag',
        '--flag',
        '--variant', 'first'
    ])
    assert proto.i32 == -12
    assert proto.s == 'hgg'
    assert proto.ui32 == 34
    assert proto.ui64 == 0
    assert proto.f == 0.
    assert list(proto.arr) == [1, 2]
    assert abs(proto.d - 3.14) < sys.float_info.epsilon
    assert proto.msg.x == 'foo'
    assert proto.flag is True


@pytest.fixture(params=['json', 'yaml', 'pb'])
def config(request):
    name = 'config.' + request.param
    conf_file_arg = '_{}_config'.format(request.param)
    conf = {'s': 'abc', 'i32': '-12', 'ui32': 12, 'msg': {'x': 'foo'}, 'req': 123}
    with open(name, 'w') as out:
        if request.param == 'json':
            json.dump(conf, out)
        elif request.param == 'yaml':
            yaml.dump(conf, out)
        elif request.param == 'pb':
            out.write('s: "abc"\ni32: -12\nui32: 12\nmsg { x: "foo"}\nreq: 123')
    return ['--config-' + request.param, name], (conf_file_arg, name)


def test_config(config):
    parser = ArgumentParser(TArgs)
    conf, (conf_file_arg, conf_file_name) = config
    args, proto = parser.parse_args(args=conf + ['--d', '3.14'])
    assert proto.i32 == -12
    assert proto.s == 'abc'
    assert proto.ui32 == 12
    assert proto.ui64 == 0
    assert proto.f == 0.
    assert abs(proto.d - 3.14) < sys.float_info.epsilon
    assert proto.msg.x == 'foo'
    assert getattr(args, conf_file_arg) == conf_file_name


def test_config_after_arg(config):
    parser = ArgumentParser(TArgs)
    conf, _ = config
    args, proto = parser.parse_args(args=['--i32', '10'] + conf)
    assert proto.i32 == -12


def test_config_before_arg(config):
    parser = ArgumentParser(TArgs)
    conf, _ = config
    args, proto = parser.parse_args(args=conf + ['--i32', '10'])
    assert proto.i32 == 10


def test_check_required():
    parser = ArgumentParser(TArgs)
    with pytest.raises(ValueError):
        args, proto = parser.parse_args(args=['--i32', '10'])
        pytest.fail('initalization of arguments errors')


def test_ignored():
    parser = ArgumentParser(TArgs)
    with pytest.raises(SystemExit):
        args, proto = parser.parse_args(args=['--ignored', 'ignored', '--req', '1'])


def test_bad_enum():
    parser = ArgumentParser(TArgs)
    with pytest.raises(SystemExit):
        args, proto = parser.parse_args(args=['--variant', 'third', '--req', '1'])
