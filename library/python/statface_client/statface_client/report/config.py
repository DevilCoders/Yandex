# coding: utf8

from __future__ import division, absolute_import, print_function, unicode_literals

import contextlib
import collections
import sys
import yaml
import six
from ..errors import StatfaceReportConfigError


# pylint: disable=too-many-instance-attributes
class StatfaceReportConfig(object):

    def __init__(  # pylint: disable=too-many-arguments
            self, title=None, user_config=None,
            dimensions=None, measures=None,
            initial_owners=None, initial_access_by_list=None,
            **params):
        """
        Examples:

            StatfaceReportConfig(
                title='Report form YAML',
                user_config=open('user_config.yaml').read(),
            )

            StatfaceReportConfig(
                title='Test report',
                dimensions=[{'fielddate': 'date'}, {'content': 'string'}],
                titles={"content": "Content"},
            )

        """
        if user_config is not None and (dimensions is not None or measures is not None):
            raise StatfaceReportConfigError(
                '`user_config` should not be specified together with'
                ' `dimensions` or `measures`')

        self.title = title
        self.initial_owners = initial_owners
        self.initial_access_by_list = initial_access_by_list
        self.misc = params

        self.user_config_source = None
        self.user_config_data = None

        self.dimensions = self._fields_to_ordered_dict(dimensions or [])
        self.measures = self._fields_to_ordered_dict(measures or [])
        if user_config is not None:
            self.update_from_user_config(user_config)

    # See:
    # https://github.yandex-team.ru/statbox/statface-api-v4/blob/40a2d2b38067551cd035d44f3ff346f97c6a49f2/statface_v4/statreport/_stuff_xreportmanager_create.py#L61
    # (`misc` because minus the user_config)
    _top_level_misc_keys = ('title', 'initial_owners', 'initial_access_by_list')

    @property
    def _top_level_misc(self):
        return {key: getattr(self, key) for key in self._top_level_misc_keys}

    def __unicode__(self):
        return self.to_yaml()

    def __repr__(self):
        if six.PY3:
            return self.__unicode__()
        return self.__unicode__().encode('utf-8')

    # TODO: proper check
    @property
    def is_valid(self):
        return self.title and self._is_dimensions_valid(self.dimensions)

    def check_valid(self):
        """Raise `StatfaceReportConfigError` if config is not valid."""
        if not self.title:
            raise StatfaceReportConfigError('invalid report config: no title')
        if not self._is_dimensions_valid(self.dimensions):
            raise StatfaceReportConfigError(
                'invalid report config: wrong dimensions\n{}'.format(
                    self.dimensions
                )
            )

    @staticmethod
    def _is_dimensions_valid(dimensions):
        return dimensions.get('fielddate') == 'date'  # TODO: check template

    @staticmethod
    def _fields_to_ordered_dict(iterable):
        try:
            return collections.OrderedDict(iterable)
        except ValueError:  # for [{1: 1}, {2: 2}] form
            result = collections.OrderedDict()
            for item in iterable:
                result.update(item)
            return result

    _to_ordered_dict = _fields_to_ordered_dict  # deprecated

    @classmethod
    # pylint: disable=too-many-locals,too-many-branches,too-many-statements
    def _parse_dict_config(cls, data, data_source=None, user_config_source=None):
        """
        Make attributes dict out of a report config in either of forms:

          * `{'title': ..., 'user_config': {'dimensions': ..., ...}, ...}`
          * `{'title': ..., 'user_config': 'dimensions:\n - fielddate: date\n...', ...}`
          * `{'dimensions': ..., ...}`

        """
        assert isinstance(data, dict)

        orig_data = data
        result = {}

        user_config = data.get('user_config')

        if user_config is None:  # ... `data` is a 'user_config'.
            if data.get('title'):  # WARNING: not recommended.
                data = data.copy()
                title = data.pop('title', None)
                if title is not None:
                    # NOTE: in this case, `title` is not set in the `result` if
                    # it is not specified (thus, `update_from_...` will keep
                    # the previous value).
                    result['title'] = title

            user_config = data
            # upper_config = dict(user_config=user_config)

            result['user_config_source'] = user_config_source or data_source
            result['user_config_data'] = orig_data
        else:  # ... `data` is an upper-config.
            upper_config = data

            # Allow YAML-serialized `user_config` in data:
            if isinstance(user_config, (six.binary_type, six.text_type)):
                user_config_source = user_config_source or user_config
                user_config = cls._config_yaml_loads(
                    user_config_source, comment="parsing user_config YAML")

            assert isinstance(user_config, dict), "user_config must be a dict at this point"
            if 'title' in user_config:
                user_config = user_config.copy()
                user_config_title = user_config.pop('title')
                if 'title' in upper_config:
                    upper_config_title = upper_config['title']
                    if isinstance(upper_config_title, six.binary_type):
                        upper_config_title = upper_config_title.decode(
                            'utf-8', errors='strict'
                        )
                    # This check is to catch `data` like `{title: tile1,
                    # user_config: {title: title2, ...}}`.
                    if user_config_title != upper_config_title:
                        raise StatfaceReportConfigError(
                            ('Both `config.title` and `config.user_config.title` were'
                             ' specified and didn\'t match.'
                             ' config.title={!r}, config.user_config.title={!r}').format(
                                 upper_config_title, user_config_title))
                else:
                    upper_config['title'] = user_config_title

            # Always used (even if null):
            for key in cls._top_level_misc_keys:  # (title, initial_...)
                result[key] = upper_config.get(key)

            result['user_config_source'] = user_config_source
            result['user_config_data'] = user_config

        try:
            dimensions = user_config['dimensions']
            measures = user_config['measures']
        except KeyError as exc:
            raise StatfaceReportConfigError(
                'insufficient dataset {} to create a config:\n'
                'dict has no field {}'.format(user_config, exc.args[0])
            )
        for item in (dimensions, measures):
            if isinstance(item, dict):
                continue
            if (isinstance(item, list) and
                    all((isinstance(subitem, dict) and len(subitem) == 1 for subitem in item))):
                continue
            try:
                not_parsable = six.text_type(item)
            except Exception:  # pylint: disable=broad-except
                not_parsable = u'not parsable config'

            raise StatfaceReportConfigError(
                u"Can't parse config part:\n{}\n".format(not_parsable))

        dimensions = cls._fields_to_ordered_dict(dimensions)
        measures = cls._fields_to_ordered_dict(measures)
        result['dimensions'] = dimensions
        result['measures'] = measures

        misc = dict(user_config)
        for key in ('dimensions', 'measures'):
            misc.pop(key, None)
        result['misc'] = misc

        return result

    def update_from_dict(self, data, **kwargs):
        """:param data: (dict)"""
        parsed_data = self._parse_dict_config(data, **kwargs)
        for key, value in parsed_data.items():
            setattr(self, key, value)

    def from_dict(self, data):
        # WARNING: deprecated
        self.update_from_dict(data)

    @staticmethod
    def _config_yaml_loads(yaml_data, comment="parsing report config YAML", **kwargs):
        with wrap_config_error(comment=comment):
            return yaml.safe_load(yaml_data, **kwargs)

    @staticmethod
    def _config_yaml_dumps(
            data, default_flow_style=False, allow_unicode=True, encoding=None,
            comment="making report config YAML", **kwargs):
        with wrap_config_error(comment=comment):
            return yaml.safe_dump(
                data,
                default_flow_style=default_flow_style,
                allow_unicode=allow_unicode,
                encoding=encoding,
                **kwargs)

    def update_from_yaml(self, yaml_data):
        """:param yaml_data: (string)"""
        if isinstance(yaml_data, six.binary_type):
            yaml_data = yaml_data.decode('utf-8', errors='strict')
        if hasattr(yaml_data, 'read'):
            yaml_data = yaml_data.read()
        data = self._config_yaml_loads(yaml_data)
        return self.update_from_dict(data, data_source=yaml_data)

    def from_yaml(self, yaml_data):
        # WARNING: deprecated
        return self.update_from_yaml(yaml_data)

    def update_from_user_config(self, user_config):
        # # TODO:
        # if isinstance(user_config, StatfaceReportConfig):
        #     self.update_from_dict(user_config.to_dict())
        if isinstance(user_config, (bytes, six.text_type)):
            return self.update_from_yaml(user_config)
        return self.update_from_dict(user_config)

    def _user_config_dict(self):
        return dict(
            self.misc,
            dimensions=[{key: value} for key, value in self.dimensions.items()],
            measures=[{key: value} for key, value in self.measures.items()],
        )

    def to_dict(self, with_source=True, require_strings=False):
        result = {}
        result['title'] = self.title
        if require_strings:
            result['user_config'] = self.user_config_yaml(allow_source=with_source)
        else:
            result['user_config'] = self._user_config_dict()
        if self.initial_owners is not None:
            result['initial_owners'] = self.initial_owners
        if self.initial_access_by_list is not None:
            result['initial_access_by_list'] = self.initial_access_by_list
        if with_source and not require_strings:
            source = self.user_config_source_verified
            if source:
                result['user_config_source'] = source
        return result

    @property
    def user_config_source_verified(self):
        """ `self.user_config_source` if it matches the current config, None otherwise """
        source = self.user_config_source
        if not source:
            return None
        original_data = self.__class__(
            user_config=source, **self._top_level_misc
        )._user_config_dict()
        if self._user_config_dict() == original_data:
            return source
        return None

    def user_config_yaml(self, allow_source=True):
        data = self.to_dict()
        # Compare the saved `self.user_config_source` with the `data`,
        # return the saved source if they match.
        source = self.user_config_source
        if allow_source:
            source = self.user_config_source_verified
            if source:
                return source
        return self._config_yaml_dumps(data['user_config'])

    def to_yaml(self):
        """:returns: (string)"""
        data = self.to_dict()
        return self._config_yaml_dumps(data)


@contextlib.contextmanager
def wrap_config_error(comment=None):
    try:
        yield None
    except Exception as exc:  # pylint: disable=broad-except
        _, _, e_tb = sys.exc_info()
        six.reraise(StatfaceReportConfigError, StatfaceReportConfigError(exc, comment), e_tb)
