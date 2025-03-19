import codecs
import re
from typing import Container
from typing import Dict
from typing import Optional
from typing import Tuple
from typing import Type
from typing import TypeVar

from schematics.exceptions import BaseError
from schematics.types import BaseType
from schematics.types import StringType

from yc_common import logging
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.clients.kikimr.sql import SqlNotIn
from yc_common.exceptions import RequestValidationError
from yc_common.formatting import camelcase_to_underscore
from yc_common.models import IsoTimestampType
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase

FILTER_REGEX = re.compile(r"^(?P<field>\w+)\s*(?P<op>!?=|>=?|<=?)\s*(?P<value>.+)$|"
                          r"^(?P<list_field>\w+)\s*(?P<list_op>IN|NOT IN)\s*\((?P<list>.+)\)$", re.IGNORECASE)

ORDER_BY_REGEX = re.compile(r"^(?P<field>\w+)(?:\s(?P<dir>ASC|DESC))?$", re.IGNORECASE)

log = logging.get_logger(__name__)


def dequote(string: str) -> str:
    """Remove quotes from around a string."""
    if ((string.startswith("\"") and string.endswith("\"")) or
            (string.startswith("'") and string.endswith("'"))):
        return string[1:-1]
    else:
        raise ValueError


BT = TypeVar("BT", bound=BaseType)


def parse_literal(field: BT, value):
    try:
        if isinstance(field, StringType):
            return field.to_native(codecs.decode(dequote(value), "unicode_escape"))
        elif isinstance(field, IsoTimestampType) and re.match(r"^\d+$", value):
            return field.to_native(int(value))
        else:
            return field.to_native(value)

    except (BaseError, ValueError):
        raise RequestValidationError(message="Invalid parameter: filter value")


def parse_list(field, value):
    list_values = [i.strip() for i in value.split(",")]
    return [parse_literal(field, i) for i in list_values]


QUOTE = {"\"", "'"}
OPERATOR = " AND "
ESCAPE = "\\"


def tokenize(string: str) -> list:
    result = []
    quoted = None
    cursor = 0
    escaped = False
    i = 0
    while i < len(string):
        c = string[i]
        if escaped:
            escaped = False
        elif c == ESCAPE:
            escaped = True
        elif c in QUOTE:
            if quoted is None:
                quoted = c
            elif not escaped:
                quoted = None
        elif string[i: i + len(OPERATOR)].upper() == OPERATOR and not quoted:
            result.append(string[cursor: i].strip())
            cursor = i + len(OPERATOR)
            i = cursor
            continue
        i += 1
    if string[cursor:]:
        result.append(string[cursor:].strip())
    return result


def _parse_filter(query: str, valid_names: Container, fields: Dict, category_field: Optional[str] = None,  # noqa: F811
                  ) -> Tuple[list, list, Optional[str]]:
    filter_query = []
    filter_args = []
    category_id = None

    if not query:
        return filter_query, filter_args, category_id

    filters = tokenize(query)

    for f in filters:
        m = FILTER_REGEX.match(f.strip())
        if m is None:
            raise RequestValidationError(message="Invalid parameter: can not parse filter")
        field_name = m.group("field")
        list_field_name = m.group("list_field")
        if field_name:
            norm_name = camelcase_to_underscore(field_name)

            if category_field and norm_name == category_field:

                category_id = parse_literal(ResourceIdType(), m.group("value"))
                continue
            elif norm_name not in valid_names:
                raise RequestValidationError(message="Invalid parameter: can not filter by '{}'".format(field_name))

            filter_query.append("{field} {op} ? ".format(field=norm_name, op=m.group("op")))
            filter_args.append(parse_literal(fields[norm_name], m.group("value")))
        elif list_field_name:
            norm_name = camelcase_to_underscore(list_field_name)
            if norm_name not in valid_names:
                raise RequestValidationError(
                    message="Invalid parameter: can not filter by '{}'".format(list_field_name))
            op = m.group("list_op")
            parsed_list_values = parse_list(fields[norm_name], m.group("list"))
            if op.upper() == "IN":
                filter_args.append(SqlIn(norm_name, parsed_list_values))
            else:
                filter_args.append(SqlNotIn(norm_name, parsed_list_values))

            filter_query.append(" ? ")

        else:
            raise RequestValidationError(message="Invalid parameter: can not parse filter")

    return filter_query, filter_args, category_id


ModelType = TypeVar("ModelType", bound=Type[AbstractMktBase])


def parse_filter(query: str, model: ModelType) -> Tuple[list, list]:
    if not model.Filterable_fields:
        raise RequestValidationError(message="Invalid parameter: filter")

    filter_query, filter_args, _ = _parse_filter(query, model.Filterable_fields, model.fields)
    return filter_query, filter_args


def parse_filter_with_category(query: str, model: ModelType, category_field="category_id") -> Tuple[list, list, str]:
    if not model.Filterable_fields:
        raise RequestValidationError(message="Invalid parameter: filter")

    return _parse_filter(query, model.Filterable_fields, model.fields, category_field)


def parse_order_by(
        s: str,
        mapping: Dict[str, str],
        default: str,
        default_dir: str = "ASC",
) -> str:
    field_name = default
    direction = " " + default_dir.strip()
    if s is None:
        s = ""
    m = ORDER_BY_REGEX.match(s)
    if m:
        field_name = m.group("field")
        d = m.group("dir")
        direction = direction if d is None else " {}".format(d.upper())

        if field_name not in mapping:
            raise RequestValidationError(message="Invalid parameter: orderBy")

    return "{field}{dir}".format(field=mapping[field_name], dir=direction)
