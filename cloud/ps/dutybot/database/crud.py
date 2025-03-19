import logging

from database.db_common import create_connection, isolate_strings


def read_query(table: str, condition=None, fields=None):
    """
    Basic read transaction function
    Returns query records `[(tuple),]`
    """
    if fields is None:
        fields = ["*"]
    fields = ",".join(fields)
    query = f"SELECT {fields} FROM {table}"
    if condition:
        query = f"SELECT {fields} FROM {table} {condition}"
    logging.info(f"executing query: {query}")
    try:
        connection = create_connection()
        cursor = connection.cursor()
        cursor.execute(query)
        output = cursor.fetchall()
        return output
    except Exception as err:
        logging.error(f"exception {err}")
        return
    finally:
        connection.close()


def update_query(table: str, condition=None, fields=None):
    """
    Basic update transaction function
    Returns `bool::query_result`
    """
    if fields is None:
        fields = {}
    fields = isolate_strings(fields)
    fields_to_update = ",".join(['%s=%s' % (field, new_value) for (field, new_value) in fields.items()])
    query = f"UPDATE {table} SET {fields_to_update}"
    if condition:
        query = f"UPDATE {table} SET {fields_to_update} {condition}"
    logging.info(f"Executing query: {query}")
    try:
        connection = create_connection()
        cursor = connection.cursor()
        cursor.execute(query)
        connection.commit()
        return True if cursor.rowcount > 0 else False
    except Exception as err:
        logging.error(f"exception {err}")
        return False
    finally:
        connection.close()


def delete_query(table: str, condition=None):
    """
    Basic delete transaction function
    Returns `bool::query_result`
    """

    query = f"DELETE FROM {table} {condition}"
    logging.info(f"executing query: {query}")
    try:
        connection = create_connection()
        cursor = connection.cursor()
        cursor.execute(query)
        connection.commit()
        return True
    except Exception as err:
        logging.error(f"exception {err}")
        return False
    finally:
        connection.close()


def create_query(table: str, data: dict = None):
    """
    Basic create transaction function
    Returns new record: `[(tuple)],`
    """
    if data is None:
        data = {}

    data = isolate_strings(data)
    fields = ",".join(data.keys())
    values = ",".join(data.values())
    query = f"INSERT INTO {table} ({fields}) VALUES ({values})"
    logging.info(f"executing query: {query}")
    try:
        connection = create_connection()
        cursor = connection.cursor()
        cursor.execute(query)
        inserted_rows = cursor.rowcount
        connection.commit()
        return True if inserted_rows > 0 else False
    except Exception as err:
        logging.error(f"exception {err}")
        return False
    finally:
        connection.close()
