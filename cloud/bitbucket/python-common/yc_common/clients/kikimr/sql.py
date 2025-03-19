"""Provides various SQL operation abstractions"""

import abc
import decimal
import inspect
import itertools
import re
import uuid
from typing import Any
from typing import Dict
from typing import List
from typing import Tuple
from typing import Union

import simplejson as json

from yc_common.clients.kikimr.client import KIKIMR_CLIENT_YDB
from yc_common.clients.kikimr.client import client_version
from yc_common.clients.kikimr.exceptions import ValidationError
from yc_common.clients.kikimr.kikimr_types import KikimrDataType
from yc_common.exceptions import LogicalError
from yc_common.misc import safe_zip
from yc_common.models import JsonStr
from yc_common.models import Model
from yc_common.models import normalize_decimal

VALUE_PLACEHOLDER = "?"
_VARIABLE_PLACEHOLDER = "$"
_KIKIMR_VARIABLE_PLACEHOLDER = "$ydb_"
_CURSOR_VARIABLE_NAME = "$ydb_cursor"
_SUBSTITUTE_RE = re.compile(r"(?<=[\s\(,=])" + re.escape(_VARIABLE_PLACEHOLDER) + r"([a-zA-Z0-9_/]+)\b")

_TYPES_TO_YDB_TYPES = {
    type(None): KikimrDataType.NULL,
    bool: KikimrDataType.BOOL,
    int: KikimrDataType.INT64,
    float: KikimrDataType.DOUBLE,
    str: KikimrDataType.UTF8,
    JsonStr: KikimrDataType.UTF8,
    uuid.UUID: KikimrDataType.UTF8,
    decimal.Decimal: KikimrDataType.DECIMAL
}


class Variable:
    def __init__(self, value, var_type: str, *, name: str = None, optional: bool = True):
        if var_type not in KikimrDataType.ALL:
            raise LogicalError("It is not kikimr type: '{}'", var_type)

        self.name = name
        self.type = var_type
        self.__optional = optional
        # we need decimal to become native at the very very end in order to prevent float values in Json
        if value is not None and var_type == KikimrDataType.DECIMAL and client_version() == KIKIMR_CLIENT_YDB:
            self.value = decimal.Decimal(value)
        else:
            self.value = value

    def __str__(self):
        return "{}, {}, {}, '{}'".format(self.name, self.type, self.__optional, self.value)

    def declare(self) -> str:
        t = self.type if self.type in KikimrDataType.ALL else _TYPES_TO_YDB_TYPES[self.type]
        if self.__optional:
            t = "\"Optional<{}>\"".format(t)

        return "DECLARE {} AS {}".format(self.name, t)


class QueryTemplate:
    __MODE_PREPARED = "prepared"
    __MODE_TEXT = "text"

    def __init__(self):
        self.text_template = ""
        self.values = {}  # type: Dict[str, Any]
        self.__description = {}  # type: Dict[str, Variable]
        self.__last_index = 0
        self.__mode = self.__MODE_PREPARED
        self.know_all_types = True

    def __str__(self):
        return self.text_template

    def __create_var_name(self) -> str:
        self.__last_index += 1
        return "$ydb_auto_{}".format(self.__last_index)

    def add_var_auto(self, var) -> str:
        """
        :param var: variable to add. May be value of var, or description.
        :return: name of added variable
        """
        if isinstance(var, Variable):
            value = var
        else:
            # Driver don't know exact type and it can raise exception while compile query
            self.know_all_types = False

            # Method doesn't know real type of variable and use stub NoneType for create query.
            # If value will not null - it will other query.
            if var is None:
                self.prefer_text_mode()
            value = Variable(var, _TYPES_TO_YDB_TYPES[type(var)])

        if value.name is None:
            value.name = self.__create_var_name()

        self.values[value.name] = self._prepare_value(value.value, value.type)
        self.__description[value.name] = value
        return value.name

    def _prepare_value(self, value, ydb_type):
        if value is None:
            return value
        if isinstance(value, JsonStr):
            return value
        if ydb_type == KikimrDataType.JSON:
            return json.dumps(value)
        return value

    def build_text_for_prepare(self) -> str:
        # Need repeatable build str for good cache
        keys = list(self.__description.keys())
        keys.sort()

        if len(keys) > 0:
            declare = "; ".join([self.__description[k].declare() for k in keys])
            declare += "; "
        else:
            declare = ""
        return declare + self.text_template

    def build_text_query(self) -> str:
        """
        Return usual str query with filled values
        :return:
        """

        rendered_values = {
            key[len(_VARIABLE_PLACEHOLDER):]: render_value(val, self._get_value_type_by_description(key))
            for key, val in self.values.items()}

        return substitute_variables(self.text_template, rendered_values)

    def prefer_text_mode(self):
        """
        Autobuilder method for prefer fallback to text query instead compiled
        :return:
        """
        self.__mode = self.__MODE_TEXT

    def is_text_mode_preferred(self):
        return self.__mode == self.__MODE_TEXT

    def _get_value_type_by_description(self, key):
        variable = self.__description.get(key)
        if variable is not None:
            return variable.type
        else:
            return None


class _InvalidQueryError(ValidationError):
    pass


class _Renderable(abc.ABC):
    @abc.abstractmethod
    def render(self, query_template: QueryTemplate) -> str:
        """Returns string representation of object suitable for YQL query"""


class _RecursiveRenderable(abc.ABC):
    @abc.abstractmethod
    def render(self) -> Tuple[str, Union[list, Tuple]]:
        """Returns string and arguments suitable for preparing YQL query"""


class _Placeholder(_Renderable):
    def render(self, variables: dict = None) -> str:
        raise LogicalError("Invalid usage of {}.", self.__class__.__name__)


class SqlQuery(_RecursiveRenderable):
    def __init__(self, query, *args):
        self.__query = query
        self.__args = args

    def render(self) -> Tuple[str, Union[list, Tuple]]:
        return self.__query, self.__args


class SqlCondition(_RecursiveRenderable):
    def __init__(self, *initial_condition):
        self.__conditions = []
        self.__arguments = []

        if initial_condition:
            self.and_condition(*initial_condition)

    @property
    def empty(self):
        return not self.__conditions

    def and_condition(self, condition, *args):
        if isinstance(condition, _Renderable):
            if args:
                raise LogicalError()

            self.__conditions.append("?")
            self.__arguments.append(condition)
        elif isinstance(condition, _RecursiveRenderable):
            sub_condition, sub_arguments = condition.render()
            self.__conditions.append(sub_condition)
            self.__arguments.extend(sub_arguments)
        else:
            self.__conditions.append(condition)
            self.__arguments.extend(args)

    def render(self) -> Tuple[str, Union[list, Tuple]]:
        if self.empty:
            return "TRUE", []
        else:
            return " AND ".join(self.__conditions), self.__arguments


class SqlWhere(SqlCondition):
    def render(self) -> Tuple[str, Union[list, Tuple]]:
        if self.empty:
            return "", []
        else:
            query, args = super().render()
            return "WHERE " + query, args


def _format_values(values):
    return "({})".format(", ".join(values))


class _SqlInBase(_Renderable):
    _default_value = None
    _keyword = None

    def __init__(self, column_name, values):
        self.__column_name = column_name
        self.__values = values

    def render(self, query_template: QueryTemplate) -> str:
        # IN query contain variable number of vars depends from arguments count it mean queries with different number
        # of variants will be different
        query_template.prefer_text_mode()

        if not self.__values:
            return self._default_value

        str_values = _format_values(_render_value_for_template(value, query_template) for value in self.__values)
        return "{} {} {}".format(self.__column_name, self._keyword, str_values)


class SqlIn(_SqlInBase):
    _default_value = "FALSE"
    _keyword = "IN"


class SqlNotIn(_SqlInBase):
    _default_value = "TRUE"
    _keyword = "NOT IN"


class _SqlInManyBase(_RecursiveRenderable):
    _default_value = None
    _keyword = None

    def __init__(self, column_names, values):
        self.__column_names = column_names
        self.__values_list = list(values)  # Use list to accept any iterable type

    def render(self) -> Tuple[str, Union[list, Tuple]]:
        if not self.__values_list:
            return self._default_value, ()

        if any(len(values) != len(self.__column_names) for values in self.__values_list):
            raise LogicalError("Iterable lengths don't match.")

        str_column_names = _format_values(self.__column_names)
        placeholder = _format_values([VALUE_PLACEHOLDER] * len(self.__column_names))
        values_placeholder = _format_values([placeholder] * len(self.__values_list))
        query = "{} {} {}".format(str_column_names, self._keyword, values_placeholder)

        return query, list(itertools.chain(*self.__values_list))


# TODO(lavrukov): Don't use before KIKIMR-5598 !
class SqlInMany(_SqlInManyBase):
    _default_value = "FALSE"
    _keyword = "IN"


# TODO(lavrukov): Don't use before KIKIMR-5598 !
class SqlNotInMany(_SqlInManyBase):
    _default_value = "TRUE"
    _keyword = "NOT IN"


class SqlInsertValues(_Renderable):
    def __init__(self, column_names, values):
        self.__column_names = column_names
        self.__values = values

    def render(self, query_template: QueryTemplate) -> str:
        return "({columns}) VALUES {values}".format(
            columns=", ".join(self.__column_names),
            values=", ".join(self.__render_row(row, query_template) for row in self.__values))

    @staticmethod
    def __render_row(row, query_template):
        return "(" + ", ".join(_render_value_for_template(value, query_template) for value in row) + ")"


class SqlInsertObject(_RecursiveRenderable):
    def __init__(self, value):
        self.__value = value

    def render(self) -> Tuple[str, Union[list, Tuple]]:
        keys = list(self.__value.keys())
        query = "({}) VALUES ({})".format(", ".join(keys), ", ".join("?" for _ in keys))
        return query, [self.__value[k] for k in keys]


# TODO(lavrukov): SqlCompoundKeyMatch = SqlInMany after KIKIMR-5598
class SqlCompoundKeyMatch(_Renderable):
    def __init__(self, column_names, values):
        self.__column_names = column_names
        self.__values = list(values)  # Use list to accept any iterable type

    def render(self, query_template: QueryTemplate) -> str:
        if not self.__values or not self.__column_names:
            return "FALSE"

        def render_comparison(key, value) -> str:
            if value is None or isinstance(value, Variable) and value.value is None:
                return "{} IS NULL".format(key)

            return "{} = {}".format(key, _render_value_for_template(value, query_template))

        return "(" + " OR ".join(
            " AND ".join(
                render_comparison(key, value)
                for key, value in safe_zip(self.__column_names, column_values)
            )
            for column_values in self.__values
        ) + ")"


class SqlCompoundKeyCursor(_RecursiveRenderable):
    def __init__(self, keys, values, ns=None, keys_desc=None):
        self.__keys = keys
        self.__values = values
        self.__ns = ns

        if keys_desc and not set(keys_desc).issubset(keys):
            raise LogicalError("Desc keys must be subset of keys.")
        self.__keys_desc = keys_desc if keys_desc else []

    def __render_key(self, key):
        if self.__ns is not None:
            return "{}.{}".format(self.__ns, key)
        else:
            return key

    def render(self) -> Tuple[str, Union[list, Tuple]]:
        final_conditions, final_args = [], []
        fixed_conditions, fixed_args = [], []
        for arg_key, value in safe_zip(self.__keys, self.__values):
            key = self.__render_key(arg_key)

            comparator = "<" if arg_key in self.__keys_desc else ">"
            inner_condition = fixed_conditions + ["{} {} ?".format(key, comparator)]
            inner_args = fixed_args + [value]

            fixed_conditions.append("{} = ?".format(key))
            fixed_args.append(value)

            final_conditions.append(" AND ".join(inner_condition))
            final_args.extend(inner_args)

        if final_conditions:
            return "(" + " OR ".join(final_conditions) + ")", final_args
        else:
            return "", []


class SqlOrder(_Renderable):
    def __init__(self, fields: Union[str, List[str]] = None):
        if isinstance(fields, str):
            fields = [fields]
        self._fields = fields

    def render(self, query_template: QueryTemplate) -> str:
        if not self._fields:
            return ""

        order_fields = []
        for field in self._fields:
            field_name = field
            order_asc = True

            if field.startswith("-"):
                field_name = field[1:]
                order_asc = False
            if field.startswith("+"):
                field_name = field[1:]
                order_asc = True

            if order_asc:
                order = "ASC"
            else:
                order = "DESC"
            order_fields.append("{} {}".format(field_name, order))
        res = ", ".join(order_fields)
        res = "ORDER BY " + res
        return res

    def reverse(self) -> "SqlOrder":
        if not self._fields:
            return SqlOrder(None)
        fields = []
        for field in self._fields:
            if field.startswith("-"):
                new_field = field[1:]
            elif field.startswith("+"):
                new_field = "-" + field[1:]
            else:
                new_field = "-" + field
            fields.append(new_field)
        return SqlOrder(fields)


class SqlCompoundKeyOrderLimit(_Renderable):
    def __init__(self, keys, limit=None, ns=None, keys_desc=None):
        self.__keys = keys
        self.__limit = limit
        self.__ns = ns

        if keys_desc and not set(keys_desc).issubset(keys):
            raise LogicalError("Desc keys must be subset of keys.")

        self.__keys_desc = keys_desc if keys_desc else []

    def __render_key(self, key):
        if self.__ns is not None:
            return "{}.{}".format(self.__ns, key)
        else:
            return key

    def render(self, query_template: QueryTemplate) -> str:
        result = []
        if self.__keys:
            order_keys = []
            for arg_key in self.__keys:
                key = self.__render_key(arg_key)
                if arg_key in self.__keys_desc:
                    key += " DESC"

                order_keys.append(key)

            result.append("ORDER BY " + ", ".join(order_keys))
        if self.__limit is not None:
            result.append("LIMIT {}".format(
                query_template.add_var_auto(Variable(self.__limit, KikimrDataType.UINT64, optional=False))))
        return " ".join(result)


class SqlCursorCondition(_Placeholder):
    def __init__(self, **fixed_primary_key_values):
        self.fixed_primary_key_values = fixed_primary_key_values


class SqlCursorOrderLimit(_Placeholder):
    pass


class SqlLimit(_Renderable):
    def __init__(self, limit=None):
        self.__limit = limit

    def render(self, query_template: QueryTemplate) -> str:
        if self.__limit is None:
            return ""
        else:
            return "LIMIT {}".format(
                query_template.add_var_auto(Variable(self.__limit, KikimrDataType.UINT64, optional=False)))


def render_decimal(value):
    return "Decimal('{:.9f}', 22, 9)".format(normalize_decimal(value)) if value is not None else "NULL"


def render_str(value: str):
    return "'" + value.replace("\\", "\\\\").replace("'", "\\'").replace("\0", r"\x00") + "'"


def render_uint32(value):
    _validate_int(value)
    return repr(value) + "U"


def render_uint64(value):
    _validate_int(value)
    return repr(value) + "UL"


def render_json(value):
    if isinstance(value, (dict, list)):
        value = json.dumps(value)
    # previously rendered json null value on export
    if value.lower() == "null":
        return "NULL"
    return "json({})".format(render_str(value))


def render_value(value, ydb_type: KikimrDataType = None):
    # TODO: Support all available types
    # FIXME: Is this escaping logic is enough?
    # FIXME: Escape ?, $ and other values, used by our client?
    # FIXME: `UPDATE ... SET column = NULL`, but `SELECT ... WHERE column is NULL`
    # TODO support cast to other datatype
    if value is None:
        return "NULL"
    elif value is True:
        return "true"
    elif value is False:
        return "false"
    elif ydb_type == KikimrDataType.DECIMAL or type(value) is decimal.Decimal:
        return render_decimal(value)
    elif ydb_type == KikimrDataType.JSON:
        return render_json(value)
    elif ydb_type == KikimrDataType.UINT32:
        return render_uint32(value)
    elif ydb_type == KikimrDataType.UINT64:
        return render_uint64(value)
    elif type(value) in (int, float):
        return repr(value)
    elif type(value) is uuid.UUID:
        return "'{!s}'".format(value)
    elif type(value) in (str, JsonStr):
        return render_str(value)

    raise ValidationError("Unsupported render value type: {}.", value.__class__.__name__)


def _render_value_for_template(value, query_template: QueryTemplate) -> str:
    """
    Render value for prepared query as variable or variables expressions

    :param value: value for render. If value is tuple (not list) it interpret val (value, value_type).
                  value_type may by one of KikimrDataType.ALL (prefer) or primitive python type.
    :param query_template: input-output accumulator for execute with prepared query
    :return: text value representation
    """
    # TODO: Support all available types
    # FIXME: Is this escaping logic is enough?
    # FIXME: Escape ?, $ and other values, used by our client?
    # FIXME: `UPDATE ... SET column = NULL`, but `SELECT ... WHERE column is NULL`

    var_value = value
    var_type = type(value)

    if var_type in _TYPES_TO_YDB_TYPES:
        name = query_template.add_var_auto(var_value)
        return name

    def _convert_dict_list_model(obj):
        if isinstance(obj, Model):
            obj = obj.to_db()

        if type(obj) == list:
            obj = [item.to_db() if isinstance(item, Model) else item for item in obj]

        return obj

    def _render_dict_list_model(obj):
        obj = _convert_dict_list_model(obj)

        try:
            return json.dumps(obj)
        except ValueError:
            raise ValidationError("Unable to encode the value as JSON.")

    if issubclass(var_type, Variable):
        # To support storing list/dict/Model objects in UTF-8/JSON columns
        if type(var_value.value) in (list, dict) or issubclass(type(var_value.value), Model):
            if var_value.type == KikimrDataType.UTF8:
                var_value.value = _render_dict_list_model(var_value.value)
            elif var_value.type == KikimrDataType.JSON:
                var_value.value = _convert_dict_list_model(var_value.value)

        return query_template.add_var_auto(var_value)
    elif var_type == bytes:
        # FIXME: Binary data should be escaped as '\x00'
        # FIXME: We log SQL requests, so binary data representation must be printable
        pass
    elif inspect.isclass(var_type) and issubclass(var_type, Model) or var_type in (list, dict):
        return _render_value_for_template(_render_dict_list_model(var_value), query_template)
    elif inspect.isclass(var_type) and issubclass(var_type, _Renderable):
        return value.render(query_template)
    elif inspect.isclass(var_type) and issubclass(var_type, _RecursiveRenderable):
        query, args = value.render()
        return _build_query_template(query, *args, query_template=query_template)
    raise ValidationError("Unsupported render value type: {}.", value.__class__.__name__)


def build_query_template(query, *args, variables=None) -> QueryTemplate:
    res = QueryTemplate()
    query_text = _build_query_template(query, *args, query_template=res, variables=variables)
    res.text_template = query_text
    return res


# FIXME: Substitution is very insecure
def _build_query_template(query, *args, query_template: QueryTemplate, variables=None) -> str:
    """
    Replace placeholders by actual vars and create text query.
    For compile prepared query use  @prepare_query_ydb
    :param query: Query template
    :param args: Argument values
    :param variables: internal values dictionary for replace in query text in client side.
    :return:
    """
    query = substitute_variables(query, variables)

    # Check unbound variables, but variables passed to kikimr
    if query.replace(_KIKIMR_VARIABLE_PLACEHOLDER, "").find(_VARIABLE_PLACEHOLDER) >= 0:
        raise _InvalidQueryError("The query contains an unbound variable.")

    if query.find(VALUE_PLACEHOLDER) < 0:
        if args:
            raise _InvalidQueryError("Invalid number of arguments is passed to the query.")
        else:
            return query
    else:
        split_query = query.split(VALUE_PLACEHOLDER)
        if len(args) != len(split_query) - 1:
            raise _InvalidQueryError("Invalid number of arguments is passed to the query.")

        prepared_query = split_query[0]
        for arg, query_part in safe_zip(args, split_query[1:]):
            prepared_query += _render_value_for_template(arg, query_template) + query_part

        return prepared_query


def substitute_variable(query, name, value):
    return substitute_variables(query, variables={name: value})


def substitute_variables(query, variables=None):
    if not variables:
        return query

    chunks = []
    pos = 0
    for match in _SUBSTITUTE_RE.finditer(query):
        [name] = match.groups()
        if name not in variables:
            continue
        i, j = match.span()
        chunks.append(query[pos:i])
        chunks.append(variables[name])
        pos = j
    chunks.append(query[pos:])
    return ''.join(chunks)


def _validate_int(value):
    if type(value) is not int:
        raise ValidationError("'{!r}' is not int", value)


class SqlBatchModification:
    def __init__(self):
        self._requests = []

    def query(self, query, *args) -> "SqlBatchModification":
        self._requests.append((query, args))
        return self

    def consume(self):
        query = "; ".join(query for query, args in self._requests)
        args = tuple(itertools.chain.from_iterable(args for query, args in self._requests))
        self._requests = []
        return query, args

    def is_empty(self):
        return not self._requests

    def request_count(self) -> int:
        return len(self._requests)


class SqlCursorSubquery(_RecursiveRenderable):
    def __init__(self, cursor_spec, query, *args):
        self.query = SqlQuery(query, *args)
        self.cursor_spec = cursor_spec

    # Render for placeholder in main query.
    # Real include of subquery must insert into query in separate code.
    def render(self) -> Tuple[str, Union[list, Tuple]]:
        return _CURSOR_VARIABLE_NAME, ()

    @property
    def variable_name(self):
        return _CURSOR_VARIABLE_NAME
