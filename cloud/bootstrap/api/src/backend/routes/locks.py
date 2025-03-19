"""Locks blueprint"""

import http
from typing import Any, Tuple

import flask
import flask_restplus

from bootstrap.api.core.locks import CanNotLockError, get_all_locks, get_lock, try_lock, unlock, extend

from .helpers.aux_resource import BootstrapResource
from .helpers.aux_response import make_response
from .helpers.models import empty_model, add_lock_model, lock_model

api = flask_restplus.Namespace("locks", description="Locks api")
api.add_model(empty_model.name, empty_model)
api.add_model(add_lock_model.name, add_lock_model)
api.add_model(lock_model.name, lock_model)


@api.route("")
class Locks(BootstrapResource):
    @make_response(api, lock_model, as_list=True)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        """Get list of currently acquired locks"""
        return [lock.to_json() for lock in get_all_locks()], http.HTTPStatus.OK

    @api.expect(add_lock_model)
    @make_response(api, lock_model, code=http.HTTPStatus.CREATED)
    def post(self) -> Tuple[Any, http.HTTPStatus]:
        """Create new lock"""
        try:
            body = flask.request.json

            lock = try_lock(
                hosts=body["hosts"], owner=flask.request.authentificated_user, description=body["description"],
                hb_timeout=body["hb_timeout"],
            )

            return lock.to_json(), http.HTTPStatus.CREATED
        except CanNotLockError as e:
            return str(e), http.HTTPStatus.LOCKED


@api.route("/<int:lock_id>")
class Lock(BootstrapResource):
    @make_response(api, lock_model)
    def get(self, lock_id: int) -> Tuple[Any, http.HTTPStatus]:
        """Get lock info"""
        return get_lock(lock_id).to_json(), http.HTTPStatus.OK

    @make_response(api, empty_model)
    def delete(self, lock_id: int) -> Tuple[Any, http.HTTPStatus]:
        """Unlock specified lock"""
        unlock(lock_id)
        return None, http.HTTPStatus.OK


@api.route("/<int:lock_id>/ping")
class PingLock(BootstrapResource):
    @make_response(api, lock_model)
    def post(self, lock_id: int) -> Tuple[Any, http.HTTPStatus]:
        """Update lock expiration date"""
        return extend(lock_id).to_json(), http.HTTPStatus.OK
