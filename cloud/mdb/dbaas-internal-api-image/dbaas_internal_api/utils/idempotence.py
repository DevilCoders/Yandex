# -*- coding: utf-8 -*-
"""
DBaaS Internal API idempotency helpers
"""

import json
from functools import wraps
from hashlib import md5
from uuid import UUID

from flask import g
from flask_restful import abort, request

from ..apis.operations import get_operation, get_operation_by_idem_id

IDEM_PARAM_NAME = 'Idempotency-Key'


def get_request_hash():
    """Produce request hash string"""
    hash_source = {
        'request': request.get_json(silent=True) or {},
        'endpoint': request.endpoint,
    }
    return md5(json.dumps(hash_source, sort_keys=True).encode('utf8')).digest()  # noqa


def supports_idempotence(fun):
    """
    Checks idempotence_id against a database,
    and if found returns an operation.
    """

    @wraps(fun)
    def idempotence_check_wrapper(*args, **kwargs):
        """
        Check for idempotence id and return operation if present
        """
        # Validating is done later, first check if is not None
        idempotence_id = request.headers.get(IDEM_PARAM_NAME)
        # This field will not be present until auth is complete
        folder = getattr(g, 'folder', {})
        user_id = getattr(g, 'user_id', None)
        if not all((idempotence_id, folder.get('folder_ext_id'), user_id)):
            # No idem-ids and/or auth done, proceed as usual.
            return fun(*args, **kwargs)
        # Validate UUID.
        try:
            idempotence_id = UUID(request.headers.get(IDEM_PARAM_NAME))
        except (AttributeError, TypeError, ValueError):
            abort(400, message='Malformed {idem_id}: must be a valid UUID'.format(idem_id=IDEM_PARAM_NAME))
        # This is somewhat expensive operation, so save hash for later reuse.
        g.request_hash = get_request_hash()
        g.idempotence_id = idempotence_id
        operation_id, request_hash = get_operation_by_idem_id(idempotence_id=idempotence_id)
        # There is IdemId, but it looks like its the first time we see it.
        # Proceed as usual -- idem id will be added later when it comes to
        # creating a task in queue.
        if operation_id is None:
            return fun(*args, **kwargs)
        # There is an operation registered with this idem id, check
        # arguments` hash.
        if request_hash != g.request_hash:
            abort(409, message='Operation requests with idempotence option set ' 'must have identical request bodies')
        operation = get_operation(operation_id, expose_all=False)
        g.metadb.commit()
        # There will be trouble if handler does not marshal with
        # OperationSchema. Luckily for us, idempotence is only supported
        # in Operation-generating handlers.
        return operation

    return idempotence_check_wrapper
