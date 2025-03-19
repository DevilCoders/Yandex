"""Snapshot client."""

from typing import List, Union

from yc_common import config
from yc_common.exceptions import ApiError
from yc_common.models import (
    Model, StringType, IntType, ListType, ModelType, DateTimeType, BooleanType
)
from yc_common.misc import drop_none

from .api import ApiClient


class SnapshotStateCode:
    CREATING = "creating"
    READY = "ready"
    FAILED = "failed"
    DELETING = "deleting"
    DELETED = "deleted"

    ALL = [CREATING, READY, FAILED, DELETING, DELETED]
    ALL_DELETED = [DELETING, DELETED]


class SnapshotErrorCodes:
    SnapshotNotFound = "ErrSnapshotNotFound"
    Internal = "ErrInternal"


class SnapshotChange(Model):
    timestamp = DateTimeType(required=True)
    real_size = IntType(required=True)


class SnapshotState(Model):
    code = StringType(choices=SnapshotStateCode.ALL, required=True)
    description = StringType()


# TODO: Merge with models.snapshots.Snapshot
class Snapshot(Model):
    id = StringType(required=True)
    created = DateTimeType()
    state = ModelType(SnapshotState, required=True)
    size = IntType()
    real_size = IntType()
    project = StringType()
    product_id = StringType()
    changes = ListType(ModelType(SnapshotChange))
    name = StringType()
    description = StringType()
    metadata = StringType()
    public = BooleanType()


class SnapshotClient:
    def __init__(self, url, project_id=None, timeout=10):
        if project_id:
            self.__client = ApiClient(url + "/v1/projects/" + project_id, timeout=timeout)
        else:
            self.__client = ApiClient(url + "/v1", timeout=timeout)

    def create(
            self, url, name, description, product_id=None,
            metadata=None, public=None
    ) -> str:
        class Result(Model):
            id = StringType(required=True)

        return self.__client.post("/snapshots", drop_none({
            "url": url,
            "product_id": product_id,
            "metadata": metadata,
            "name": name,
            "description": description,
            "public": public,
        }), model=Result).id

    def list(
        self, billing_start=None, billing_end=None, search_prefix=None,
        search_full=None, created_after=None, created_before=None,
        sort=None, limit=None
    ) -> List[Snapshot]:
        class Response(Model):
            result = ListType(ModelType(Snapshot), default=list)

        params = drop_none(dict(
            billing_start=billing_start, billing_end=billing_end,
            search_prefix=search_prefix, search_full=search_full,
            created_after=created_after, created_before=created_before,
            sort=sort, limit=limit
        ))

        return self.__client.get("/snapshots", model=Response,
                                 params=params if params else None).result

    def get(self, id, only_if_exists=False) -> Union[Snapshot, None]:
        try:
            return self.__client.get("/snapshots/" + id, model=Snapshot)
        except ApiError as e:
            if only_if_exists and e.code == SnapshotErrorCodes.SnapshotNotFound:
                return
            else:
                raise

    def delete(self, id, only_if_exists=False):
        try:
            self.__client.delete("/snapshots/" + id)
        except ApiError as e:
            if only_if_exists and e.code == SnapshotErrorCodes.SnapshotNotFound:
                return True
            else:
                raise
        else:
            return True


class SnapshotPrivateClient:
    def __init__(self, url):
        self.__client = ApiClient(url)

    def ping(self):
        self.__client.get("/ping")


class SnapshotEndpointConfig(Model):
    url = StringType(required=True)


def get_snapshot_client(project_id=None) -> SnapshotClient:
    return SnapshotClient(config.get_value("endpoints.snapshot", model=SnapshotEndpointConfig).url,
                          project_id)


def get_snapshot_private_client() -> SnapshotPrivateClient:
    return SnapshotPrivateClient(config.get_value("endpoints.snapshot", model=SnapshotEndpointConfig).url)
