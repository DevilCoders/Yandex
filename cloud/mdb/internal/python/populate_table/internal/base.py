"""
Fill table from json file
"""

import json
from collections.abc import Mapping

import psycopg2
import yaml
from yaml import CLoader
from dataclasses import dataclass
from psycopg2.extensions import adapt
from psycopg2.extras import NumericRange, RealDictCursor


def _int8range_adapt(value):
    return NumericRange(value[0], value[1])


CUSTOM_FIELDS = {'int8range': _int8range_adapt}


def modifier_adapt(value):
    """
    Special workaround for %-values and custom fields
    """
    if isinstance(value, Mapping):
        for key, val in value.items():
            if key not in CUSTOM_FIELDS:
                break
            return CUSTOM_FIELDS[key](val)
        return json.dumps(value).replace('''%''', '''%%''')  # noqa
    return value


def mogrify(query, kwargs):
    """
    Mogrify query (without working connection instance)
    """
    return query % {key: adapt(modifier_adapt(value)).getquoted().decode('utf-8') for key, value in kwargs.items()}


def get_conn(dsn):
    """
    Get postgresql connection
    """
    return psycopg2.connect(dsn, cursor_factory=RealDictCursor)


def diff_values(old_values, new_values, keys):
    """
    Returns 3 lists: values to remove, values to update, values to create
    """
    new_names = {tuple(x[_key] for _key in keys) for x in new_values}
    old_names = {tuple(x[_key] for _key in keys) for x in old_values}
    remove = old_names.difference(new_names)
    create = new_names.difference(old_names)
    update = []
    old_values_dict = {tuple(x[_key] for _key in keys): x for x in old_values}
    new_values_dict = {tuple(x[_key] for _key in keys): x for x in new_values}
    for name in new_names.intersection(old_names):
        if old_values_dict[name] != new_values_dict[name]:
            update.append(new_values_dict[name])
    return remove, update, [new_values_dict[x] for x in create]


@dataclass
class PopulationResult:
    created: int = 0
    updated: int = 0
    deleted: int = 0
    ALL = 2**32


def populate(cursor, table, input_file, key, refill) -> PopulationResult:
    """
    Ensure that table content corresponds with file
    or fail
    """
    result = PopulationResult()
    with open(input_file) as fobj:
        new_values = yaml.load(fobj, Loader=CLoader)
    if not new_values or refill:
        result.deleted = result.ALL
        cursor.execute('DELETE FROM {table}'.format(table=table))
        if not new_values:
            return result
    columns = set(list(new_values[0].keys()))
    for value in new_values:
        if set(list(value.keys())).difference(columns):
            raise RuntimeError(
                'Value {value} has incompatible columns ({columns})'.format(value=value, columns=columns)
            )

    if refill:
        for value in new_values:
            columns = sorted(list(value.keys()))
            mogrified = [mogrify('%(value)s', {'value': value[x]}) for x in columns]
            cursor.execute(
                'INSERT INTO {table} ({columns}) VALUES ({values})'.format(
                    table=table, columns=', '.join(columns), values=', '.join(mogrified)
                )
            )
        result.created = len(new_values)
    else:
        query = 'SELECT {columns} FROM {table}'.format(columns=', '.join(columns), table=table)
        cursor.execute(query)
        old_values = cursor.fetchall()
        keys = key.split(',')
        remove, update, create = diff_values(old_values, new_values, keys)

        for value in remove:
            condition = " AND ".join(
                [mogrify('{key} = %(value)s'.format(key=keys[i]), {'value': value[i]}) for i in range(len(keys))]
            )
            cursor.execute('DELETE FROM {table} WHERE {condition}'.format(table=table, condition=condition))
        for value in update:
            mogrified = [
                mogrify('{key} = %(value)s'.format(key=x), {'value': y}) for x, y in value.items() if x not in keys
            ]
            condition = " AND ".join(["{key} = %({key})s".format(key=_key) for _key in keys])
            condition_dict = {_key: value[_key] for _key in value if _key in keys}
            cursor.execute(
                'UPDATE {table} SET {pairs} WHERE {condition}'.format(
                    table=table, pairs=', '.join(mogrified), condition=condition
                ),
                condition_dict,
            )
        for value in create:
            columns = sorted(list(value.keys()))
            mogrified = [mogrify('%(value)s', {'value': value[x]}) for x in columns]
            cursor.execute(
                'INSERT INTO {table} ({columns}) VALUES ({values})'.format(
                    table=table, columns=', '.join(columns), values=', '.join(mogrified)
                )
            )
        result.created = len(create)
        result.updated = len(update)
        result.deleted = len(remove)
    return result
