from click import ClickException

from cloud.mdb.cli.dbaas.internal.db import db_query


def get_operation(ctx, operation_id):
    query = """
        SELECT
            operation_id,
            idempotency_key,
            operation_type,
            status,
            comment,
            author,
            instance_id,
            created_at,
            modified_at,
            explanation,
            operation_log,
            operation_state,
            executed_step_names
        FROM cms.instance_operations
        WHERE operation_id = %(operation_id)s
        """
    result = db_query(ctx, 'cmsdb', query, operation_id=operation_id)

    if not result:
        raise ClickException(f'Operation "{operation_id}" not found.')

    return result[0]


def get_operations(ctx, limit=None):
    query = """
        SELECT
            operation_id,
            idempotency_key,
            operation_type,
            status,
            comment,
            author,
            instance_id,
            created_at,
            modified_at,
            explanation,
            operation_log,
            operation_state,
            executed_step_names
        FROM cms.instance_operations
        ORDER BY created_at DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(ctx, 'cmsdb', query, limit=limit)
