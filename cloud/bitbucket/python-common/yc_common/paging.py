"""Utilities for paging"""


import functools

from yc_common import exceptions
from yc_common.clients.kikimr import sql, util as kikimr_util, client as kikimr_client


def iter_items(func, entity_name, page_size, *args, **kwargs):
    """
    Iterate over items from listing function that supports paging

    :param func: callable that returns list of entities
    :param entity_name: entity name to extract from response
    :param page_size: number of items per listing result
    :param args: `func` args
    :param kwargs: `func` kwargs
    :return: Generator[Any]
    """
    page_token = None
    while True:
        response = func(*args, page_token=page_token, page_size=page_size, **kwargs)

        try:
            response_items = getattr(response, entity_name)
        except AttributeError:
            response_items = response.items

        for item in response_items:
            yield item
        page_token = response.next_page_token
        if page_token is None:
            break


def read_page(lister, cursor, limit, items, *args, **kwargs):
    """
    Helper wrapper over listing function

    Listing function can return less results than limit due to filtering.
    We are making additional queries while resulting page less than limit
    """

    response = None
    while True:
        partial_response = lister(cursor, limit, *args, **kwargs)
        if response is None:
            response = partial_response
        else:
            response.next_page_token = partial_response.next_page_token
            getattr(response, items).extend(getattr(partial_response, items))

        response_items = getattr(response, items)
        if limit is None:
            break
        elif response.next_page_token is None or len(response_items) == limit:
            break
        elif len(response_items) > limit:
            setattr(response, items, response_items[:limit])
            response.next_page_token = getattr(response, items)[-1].id
            break
        else:
            cursor = response.next_page_token

    return response


def page_handler(func=None, items="items"):
    """
    Helper wrapper over listing function

    Parse cursor and limit from query args and run read_page method
    with specified listing function
    """

    if func is None:
        return functools.partial(page_handler, items=items)

    @functools.wraps(func)
    def wrapper(query_args, *args, **kwargs):
        cursor = query_args.get("page_token")
        limit = query_args.get("page_size")
        limit = None if limit is None else int(limit)
        return read_page(func, cursor, limit, items, *args, **kwargs)

    return wrapper


api2_page_handler = page_handler  # FIXME: remove after transition period


def _make_cursor(table_name, cursor_columns, cursor_values):
    if len(cursor_columns) > 1:
        raise exceptions.LogicalError("Multiple cursor columns are not supported yet.")

    return cursor_values[0]


def _parse_cursor(table_name, cursor_columns, cursor_values):
    if len(cursor_columns) > 1:
        raise exceptions.LogicalError("Multiple cursor columns are not supported yet.")

    return [cursor_values]


def select_one_page(cursor_spec: kikimr_client.KikimrCursorSpec, tx, query, *args,
                    model=None, validate=False, strip_table_from_columns=None, strip_cursor_columns=True,
                    make_cursor=_make_cursor, parse_cursor=_parse_cursor):

    args = list(args)
    primary_key_columns = cursor_spec.table.initial_spec().primary_keys
    cursor_limit = cursor_spec.cursor_limit
    cursor_values = cursor_spec.cursor_value
    cursor_prefix = "" if cursor_spec.table_alias is None else "{}.".format(cursor_spec.table_alias)
    table_name = cursor_spec.table.name

    if not primary_key_columns:
        raise exceptions.LogicalError()

    if strip_table_from_columns is None and model is not None:
        strip_table_from_columns = kikimr_util.ColumnStrippingStrategy.STRIP

    where_condition_index = None
    order_limit_index = None
    for index, arg in enumerate(args):
        if isinstance(arg, sql.SqlCursorCondition):
            if where_condition_index is not None:
                raise exceptions.LogicalError("Invalid paged query specification.")

            where_condition_index = index
        elif isinstance(arg, sql.SqlCursorOrderLimit):
            if order_limit_index is not None:
                raise exceptions.LogicalError("Invalid paged query specification.")

            order_limit_index = index

    if where_condition_index is None or order_limit_index is None or order_limit_index <= where_condition_index:
        raise exceptions.LogicalError("Invalid paged query specification.")

    cursor_columns = primary_key_columns[:]
    where_condition = args[where_condition_index]
    for column in where_condition.fixed_primary_key_values.keys():
        try:
            cursor_columns.remove(column)
        except ValueError:
            raise exceptions.LogicalError("Invalid paged query specification: Primary key doesn't contain {!r} column.",
                                          column)

    real_where_condition = sql.SqlCondition()
    for column, value in where_condition.fixed_primary_key_values.items():
        real_where_condition.and_condition("{} = ?".format(column), value)

    if cursor_values is not None:
        cursor_values = parse_cursor(table_name, cursor_columns, cursor_values)
        real_where_condition.and_condition(sql.SqlCompoundKeyCursor(cursor_columns, cursor_values))

    args[where_condition_index] = real_where_condition
    args[order_limit_index] = sql.SqlCompoundKeyOrderLimit(primary_key_columns, cursor_limit)

    next_page_token = None
    items = []
    for n, row in enumerate(tx.select(query, *args)):
        if cursor_limit is not None and n == cursor_limit - 1:
            next_page_token = make_cursor(table_name, cursor_columns, [row[cursor_prefix + c] for c in cursor_columns])

        # It's possibly a race in join without transaction
        if cursor_spec.join_column is not None and row[cursor_spec.join_column] is None:
            continue

        if strip_cursor_columns and cursor_prefix:
            _strip_cursor_columns(row, cursor_prefix)
        if strip_table_from_columns:
            kikimr_util.strip_table_name_from_row(row, strip_table_from_columns)

        if model is not None:
            items.append(model.from_kikimr(row, validate=validate))
        else:
            items.append(row)

    return items, next_page_token


def _strip_cursor_columns(row, cursor_prefix):
    """Remove cursor columns from row and strip table name from column name"""

    for column_name in list(row.keys()):
        if column_name.startswith(cursor_prefix):
            del row[column_name]
