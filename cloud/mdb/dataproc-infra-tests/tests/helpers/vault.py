"""
Tool for getting secrets from Vault
"""
import base64
import io
import os
import random
import re
import shlex
import string
import subprocess
from collections import OrderedDict
from shutil import copyfile
from unittest.mock import MagicMock

import jinja2
import six
import yaml
from jinja2 import nodes
from jinja2.environment import TemplateModule
from jinja2.ext import Extension
from retrying import retry
from library.python.vault_client.errors import ClientError
from library.python.vault_client.instances import Production

from .utils import get_env_without_python_entry_point


YAV_RE = re.compile(r'^(?P<uuid>(?:sec|ver)-[0-9a-z]{26,})(?:\[(?P<keys>.+?)\])?$', re.I)
STAGING_PATH = 'staging/code/salt/srv/pillar/'
SOURCE_PATH = '../salt/pillar/'


def yaml_dquote(text):
    """
    Make text into a double-quoted YAML string with correct escaping
    for special characters.  Includes the opening and closing double
    quote characters.
    """

    with io.StringIO() as ostream:
        yemitter = yaml.emitter.Emitter(ostream, width=six.MAXSIZE)
        yemitter.write_double_quoted(six.text_type(text))
        return ostream.getvalue()


def yaml_encode(data):
    """
    A simple YAML encode that can take a single-element datatype and return
    a string representation.
    """

    yrepr = yaml.representer.SafeRepresenter()
    ynode = yrepr.represent_data(data)
    if not isinstance(ynode, yaml.ScalarNode):
        raise TypeError("yaml_encode() only works with YAML scalar data; failed for {0}".format(type(data)))

    tag = ynode.tag.rsplit(":", 1)[-1]
    ret = ynode.value

    if tag == "str":
        ret = yaml_dquote(ynode.value)

    return ret


def regex_replace(data, pattern, replacement):
    """
    To replace regexp pattern string onto replacement
    """
    return re.sub(pattern, replacement, data)


@retry(wait_fixed=1000, stop_max_attempt_number=3)
def get_oauth_token(ya_command):
    cmd = f"{ya_command} vault oauth"
    result = subprocess.check_output(shlex.split(cmd), env=get_env_without_python_entry_point())
    return result.decode().strip()


def random_word(length=16):
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(length))


def get_pillars(path):
    fls = []
    for (dirpath, dirnames, filenames) in os.walk(path):
        # Ignore mdb_s3_israel and mdb_israel pillars,
        # them use lockbox and certificate-managers modules.
        if re.search('mdb.*_israel', dirpath):
            continue
        for fl in filenames:
            if fl.endswith('.sls') or fl.endswith('.jinja'):
                fls.append(os.path.join(dirpath, fl))
    return fls


class YandexVault(object):
    def __init__(self, oauth=None, ya_command=None):
        ya_command = ya_command or 'ya'
        if not oauth:
            oauth = get_oauth_token(ya_command)
        self.client = Production(authorization=f'Oauth {oauth}')
        self._cache = {}

    def _parse_key(self, key):
        matches = YAV_RE.match(key.strip())
        if not matches:
            return key
        secret_uuid = matches.group('uuid')
        keys = None
        if matches.group('keys'):
            keys = [s.strip() for s in matches.group('keys').split(',')]
        return secret_uuid, keys

    def _get_value(self, secret_uuid):
        if secret_uuid in self._cache:
            return self._cache[secret_uuid]
        try:
            value = self.client.get_version(secret_uuid, packed_value=False)['value']
        except ClientError:
            value = []
        packed_value = OrderedDict()
        for v in value:
            processed_value = v['value']
            encoding = v.get('encoding')
            if encoding and encoding == 'base64':
                processed_value = base64.b64decode(processed_value).decode('utf-8')
            packed_value[v['key']] = processed_value
        self._cache[secret_uuid] = packed_value
        return packed_value

    def _process_value(self, secret_uuid, value, keys=None):
        if not keys:
            return value

        result = {}
        for k in keys:
            if k not in value:
                value[k] = f'{secret_uuid}-{k}-{random_word()}'
            result[k] = value[k]

        if len(keys) == 1:
            return result.get(keys[0])
        return result

    def get(self, key):
        secret_uuid, keys = self._parse_key(key)
        value = self._get_value(secret_uuid)
        result = self._process_value(secret_uuid, value, keys)
        return result


def make_private_pillar(state, conf, **_):
    """
    Rewrite salt pillars with yav function
    """
    _make_private_pillar(state, conf, '../salt/pillar/', STAGING_PATH)


class SaltLoader(jinja2.BaseLoader):
    def get_source(self, environment, template):
        # Replace absolute path on salt-master with relative path on codebase
        path = template.replace('/srv/', '../salt/')
        if not path.startswith('/') and not path.startswith('..'):
            path = f'{SOURCE_PATH}{path}'
        if not os.path.exists(path):
            raise Exception(f'Template not found {path}')
        mtime = os.path.getmtime(path)
        with open(path) as f:
            source = f.read()
        return source, path, lambda: mtime == os.path.getmtime(path)


class SerializerExtension(Extension):
    tags = {"import_yaml"}

    def __init__(self, environment):
        super().__init__(environment)
        self.environment.filters.update(
            {
                "load_yaml": self.load_yaml,
            }
        )

    _load_parsers = {"load_yaml", "load_json", "load_text"}
    _import_parsers = {"import_yaml", "import_json", "import_text"}

    def parse(self, parser):
        if parser.stream.current.value in self._load_parsers:
            return self.parse_load(parser)
        elif parser.stream.current.value in self._import_parsers:
            return self.parse_import(
                parser,
                parser.stream.current.value.split("_", 1)[1],
            )

        parser.fail(
            "Unknown format " + parser.stream.current.value,
            parser.stream.current.lineno,
        )

    def parse_load(self, parser):
        filter_name = parser.stream.current.value
        lineno = next(parser.stream).lineno
        if filter_name not in self.environment.filters:
            parser.fail("Unable to parse {}".format(filter_name), lineno)

        parser.stream.expect("name:as")
        target = parser.parse_assign_target()
        macro_name = "_" + parser.free_identifier().name
        macro_body = parser.parse_statements(("name:endload",), drop_needle=True)

        return [
            nodes.Macro(macro_name, [], [], macro_body).set_lineno(lineno),
            nodes.Assign(
                target,
                nodes.Filter(
                    nodes.Call(
                        nodes.Name(macro_name, "load").set_lineno(lineno),
                        [],
                        [],
                        None,
                        None,
                    ).set_lineno(lineno),
                    filter_name,
                    [],
                    [],
                    None,
                    None,
                ).set_lineno(lineno),
            ).set_lineno(lineno),
        ]

    def load_yaml(self, value):
        if isinstance(value, TemplateModule):
            value = str(value)
        try:
            return yaml.safe_load(value)
        except yaml.YAMLError as exc:
            msg = "Encountered error loading yaml: "
            try:
                problem = exc.problem
            except AttributeError:
                # No context information available in the exception, fall back
                # to the stringified version of the exception.
                msg += str(exc)
            else:
                msg += "{}\n".format(problem)
            raise Exception(msg)
        except AttributeError:
            raise Exception("Unable to load yaml from {}".format(value))

    def parse_import(self, parser, converter):
        import_node = parser.parse_import()
        target = import_node.target
        lineno = import_node.lineno

        body = [
            import_node,
            nodes.Assign(
                nodes.Name(target, "store").set_lineno(lineno),
                nodes.Filter(
                    nodes.Name(target, "load").set_lineno(lineno),
                    "load_{}".format(converter),
                    [],
                    [],
                    None,
                    None,
                ).set_lineno(lineno),
            ).set_lineno(lineno),
        ]
        return body


class GrainsMock:
    def __init__(self, compute_conf):
        self._grains = {'id': compute_conf.get('fqdn')}

    def __getitem__(self, key):
        return self._grains[key]

    def filter_by(self, *args, **kwargs):
        return self._grains

    def get(self, key, default=None):
        return self._grains.get(key, default)


def _make_private_pillar(state, conf, in_path, out_path, **_):
    pillars = get_pillars(in_path)
    if not os.path.exists(out_path):
        os.makedirs(out_path)

    mtime_path = os.path.join(out_path, '.mtime')
    old_mtime = -1
    if os.path.exists(mtime_path):
        with open(mtime_path, 'r') as f:
            old_mtime = int(f.read())

    mtimes = [os.path.getmtime(os.path.join(x[0], filename)) for x in os.walk(in_path) for filename in x[2]]
    new_mtime = int(max(mtimes, default=0))

    if new_mtime <= old_mtime:
        return

    common = conf['common']
    yav = YandexVault(oauth=common['yav_oauth'], ya_command=common['ya_command'])

    def grains_get(*args, **kwargs):
        return ''

    salt_mock = MagicMock()
    salt_mock.yav = yav
    salt_mock.grains = GrainsMock(compute_conf=conf['compute_driver'])
    salt_mock.__getitem__ = lambda key: grains_get

    env = jinja2.Environment(loader=SaltLoader(), extensions=[SerializerExtension, 'jinja2.ext.do'])
    env.filters['json'] = env.filters['tojson']
    env.filters['yaml_dquote'] = yaml_dquote
    env.filters['yaml_encode'] = yaml_encode
    env.filters['regex_replace'] = regex_replace

    result_paths = []
    for pillar in pillars:
        result_path = pillar.replace(in_path, out_path)
        result_dir = os.path.dirname(result_path)
        if not os.path.exists(result_dir):
            os.makedirs(result_dir)
        copyfile(pillar, result_path)
        result_paths.append(result_path)

    for result_path in result_paths:
        if result_path.endswith('.sls'):
            try:
                tmpl = env.from_string(open(result_path, 'r').read())
                tmpl.stream(salt=salt_mock).dump(result_path)
            except jinja2.exceptions.TemplateAssertionError as exc:
                raise Exception(f"Wrong jinja template from pillar: {result_path}") from exc

    with open(mtime_path, 'w') as f:
        f.write(str(new_mtime))
