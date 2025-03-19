"""Utilities for paging"""
from typing import Dict
from typing import Optional
from typing import Tuple
from typing import TypeVar

from yc_common.exceptions import RequestValidationError
from yc_common.formatting import camelcase_to_underscore
from yc_common.models import Model
from yc_common.validation import ResourceIdType


def page_query_args(cursor: Optional[str],
                    limit: Optional[int],
                    id: str = "id",
                    filter_query: str = "",
                    filter_args: Optional[list] = None,
                    order_by: Optional[str] = None) -> Tuple[str, list]:
    """
    Prepare arguments for sql query for paging

    :return:
    :param cursor: start position
    :param limit: maximum number of elements to return
    :param id: name of primary key in query
    :param filter_query: additional filtration query
    :param filter_args: arguments for additional filtration query
    :param where: SqlWhere object for additional where statements
    :param order_by: order by arbitrary field
    :return: tuple (query_text, query_argument)
    """

    where_query = ""
    where_args = []

    filter_args = [] if filter_args is None else filter_args

    if cursor is not None:
        # copy arguments before modification to avoid side effects between method calls
        if filter_query:
            filter_query = "({}) AND ".format(filter_query)
        filter_query = filter_query + "{} > ?".format(id)
        filter_args = filter_args[:] + [cursor]

    if filter_query:
        where_query += " WHERE {}".format(filter_query)
        where_args.extend(filter_args)

    if order_by is None:
        where_query += " ORDER BY {}".format(id)
    else:
        where_query += " ORDER BY {}, {}".format(order_by, id)

    if limit is not None:
        where_query += " LIMIT ?"
        where_args.append(limit)

    return where_query, where_args


ModelType = TypeVar("ModelType", bound=Model)


def page_query_args_with_complex_cursor(
        cursor: ResourceIdType,
        cursor_obj: ModelType,
        limit: int,
        mapping: Dict[str, str],
        order_by: str,
        id: str = "id",
        filter_query: str = "",
        filter_args: Optional[list] = None) -> Tuple[str, list]:
    """
    Prepare arguments for sql query for paging

    :param cursor_obj:
    :param cursor: start position
    :param limit: maximum number of elements to return
    :param mapping: map cursor values
    :param id: name of primary key in query
    :param filter_query: additional filtration query
    :param filter_args: arguments for additional filtration query
    :param order_by: order by arbitrary field
    :return: tuple (query_text, query_argument)
    """
    reverse_map = {v: k for k, v in mapping.items()}
    where_query = ""
    where_args = []

    filter_args = [] if filter_args is None else filter_args
    order_by_field, order_by_dir = order_by.split(" ")
    if order_by_dir == "DESC":
        op = "<"
    else:
        op = ">"
    if cursor is not None:
        # copy arguments before modification to avoid side effects between method calls
        if filter_query:
            filter_query = "({}) ".format(filter_query)
        clause = "(({field} {op} ?) OR ({field} = ? AND {id} > ? ))".format(field=order_by_field,
                                                                            op=op,
                                                                            id=mapping[id])
        filter_query = " AND ".join([
            filter_query,
            clause,
        ])
        try:
            cursor_order_by_val = cursor_obj[camelcase_to_underscore(reverse_map[order_by_field])]
        except KeyError:
            raise RequestValidationError(message="Invalid parameter: orderBy")
        filter_args = filter_args[:] + [cursor_order_by_val, cursor_order_by_val, cursor]

    if filter_query:
        where_query += " WHERE {}".format(filter_query)
        where_args.extend(filter_args)

    if order_by is None:
        where_query += " ORDER BY {}".format(id)
    else:
        where_query += " ORDER BY {}, {}".format(order_by, id)

    if limit is not None:
        where_query += " LIMIT ?"
        where_args.append(limit)

    return where_query, where_args
