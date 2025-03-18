import collections
import marshmallow
from six.moves import collections_abc


class EnumField(marshmallow.fields.String):

    default_error_messages = {
        'invalid_enum': 'Not a valid enum identifier',
        'invalid_enum_name': 'Not a valid enum name'
    }

    def __init__(self, enum_, unknown_enum_type, **kwargs):
        super(EnumField, self).__init__(**kwargs)
        self.enum_ = enum_
        self.unknown_enum_type = unknown_enum_type

    def _deserialize(self, value, attr, data, **kwargs):
        field = super(EnumField, self)._deserialize(value, attr, data)

        if field is None:
            return None
        try:
            return self.enum_[field]
        except KeyError:
            return self.unknown_enum_type(name=field)

    def _serialize(self, value, attr, obj, **kwargs):
        if value is None:
            return super(EnumField, self)._serialize(None, attr, obj)
        if value not in self.enum_:
            self.fail('invalid_enum')
        return super(EnumField, self)._serialize(value.name, attr, obj)


class StructuredDict(marshmallow.fields.Field):
    # TODO: This is copypaste from https://github.com/marshmallow-code/marshmallow/blob/dev/marshmallow/fields.py
    # TODO: Update arcadia marshmallow
    """A dict field. Supports dicts and dict-like objects. Optionally composed
    with another `Field` class or instance.
    Example: ::
        numbers = fields.Dict(values=fields.Float(), keys=fields.Str())
    :param Field values: A field class or instance for dict values.
    :param Field keys: A field class or instance for dict keys.
    :param kwargs: The same keyword arguments that :class:`Field` receives.
    .. note::
        When the structure of nested data is not known, you may omit the
        `values` and `keys` arguments to prevent content validation.
    .. versionadded:: 2.1.0
    """

    default_error_messages = {
        'invalid': 'Not a valid mapping type.',
    }

    def __init__(self, values=None, keys=None, **kwargs):
        super(StructuredDict, self).__init__(**kwargs)
        if values is None:
            self.value_container = None
        elif isinstance(values, type):
            if not issubclass(values, marshmallow.fields.FieldABC):
                raise ValueError(
                    '"values" must be a subclass of '
                    'marshmallow.base.FieldABC',
                )
            self.value_container = values()
        else:
            if not isinstance(values, marshmallow.fields.FieldABC):
                raise ValueError(
                    '"values" must be of type '
                    'marshmallow.base.FieldABC',
                )
            self.value_container = values
        if keys is None:
            self.key_container = None
        elif isinstance(keys, type):
            if not issubclass(keys, marshmallow.fields.FieldABC):
                raise ValueError(
                    '"keys" must be a subclass of '
                    'marshmallow.base.FieldABC',
                )
            self.key_container = keys()
        else:
            if not isinstance(keys, marshmallow.fields.FieldABC):
                raise ValueError(
                    '"keys" must be of type '
                    'marshmallow.base.FieldABC',
                )
            self.key_container = keys

    def _add_to_schema(self, field_name, schema, **kwargs):
        super(StructuredDict, self)._add_to_schema(field_name, schema)
        if self.value_container:
            self.value_container.parent = self
            self.value_container.name = field_name
        if self.key_container:
            self.key_container.parent = self
            self.key_container.name = field_name

    def _serialize(self, value, attr, obj, **kwargs):
        if value is None:
            return None
        if not self.value_container and not self.key_container:
            return value
        if isinstance(value, collections_abc.Mapping):
            values = value.values()
            if self.value_container:
                values = [self.value_container._serialize(item, attr, obj) for item in values]
                if issubclass(self.value_container.__class__, marshmallow.fields.Number) and self.value_container.as_string:
                    values = [self.value_container._to_string(v) for v in values]

            keys = value.keys()
            if self.key_container:
                keys = [self.key_container._serialize(key, attr, obj) for key in keys]
                if issubclass(self.key_container.__class__, marshmallow.fields.Number) and self.key_container.as_string:
                    keys = [self.key_container._to_string(k) for k in keys]

            return dict(zip(keys, values))
        self.fail('invalid')

    def _deserialize(self, value, attr, data, **kwargs):
        if not isinstance(value, collections_abc.Mapping):
            self.fail('invalid')
        if not self.value_container and not self.key_container:
            return value

        errors = collections.defaultdict(dict)
        values = list(value.values())
        keys = list(value.keys())
        if self.key_container:
            for idx, key in enumerate(keys):
                try:
                    keys[idx] = self.key_container.deserialize(key)
                except marshmallow.ValidationError as e:
                    errors[key]['key'] = e.messages
        if self.value_container:
            for idx, item in enumerate(values):
                try:
                    values[idx] = self.value_container.deserialize(item)
                except marshmallow.ValidationError as e:
                    values[idx] = e.data
                    key = keys[idx]
                    errors[key]['value'] = e.messages
        result = dict(zip(keys, values))

        if errors:
            raise marshmallow.ValidationError(errors, data=result)

        return result


class RemoveUTCDateTimeField(marshmallow.fields.DateTime):
    def _deserialize(self, value, attr, data, **kwargs):
        """
        Workaround for https://st.yandex-team.ru/REACTOR-818
        """
        if value.endswith("[UTC]"):
            value = value[:-len("[UTC]")]
        return super(RemoveUTCDateTimeField, self)._deserialize(value, attr, data)
