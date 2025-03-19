from typing import Optional

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import builds_table
from cloud.marketplace.common.yc_marketplace_common.models.build import Build as BuildScheme
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildList
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildMetadata
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildOperation
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildStatus
from cloud.marketplace.common.yc_marketplace_common.utils.errors import BuildNotFoundError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from yc_common import logging
from yc_common.misc import timestamp
from yc_common.paging import page_handler
from yc_common.validation import ResourceIdType


log = logging.get_logger('yc')


class Build:
    @staticmethod
    @mkt_transaction()
    def get(build_id: ResourceIdType, *, tx) -> BuildScheme:
        """Internal snake_case get"""
        build = tx.with_table_scope(builds_table).select_one(
            "SELECT " + BuildScheme.db_fields() + " " +
            "FROM $table "
            "WHERE id = ?", build_id,
            model=BuildScheme)
        if build is None:
            raise BuildNotFoundError()

        return build

    @classmethod
    @mkt_transaction()
    def rpc_get(cls, id, *, tx):
        """Get for API requests"""
        return cls.get(id, tx=tx).to_public()

    @staticmethod
    @mkt_transaction()
    @page_handler(items="builds")
    def rpc_list(cursor: Optional[str],
                 limit: Optional[int] = 100,
                 *,
                 blueprint_id=None,
                 blueprint_commit_hash=None,
                 order_by: Optional[str] = None,
                 filter_query: Optional[str] = None,
                 tx=None) -> BuildList:
        filter_query, filter_args = parse_filter(filter_query, BuildScheme)

        if blueprint_id:
            filter_query += ["blueprint_id = ?"]
            filter_args += [blueprint_id]

        if blueprint_commit_hash:
            filter_query += ["blueprint_commit = ?"]
            filter_args += [blueprint_commit_hash]

        filter_query = " AND ".join(filter_query)

        mapping = {
            "id": "id",
            "blueprintId": "blueprint_id",
            "createdAt": "created_at",
            "updatedAt": "updated_at",
            "blueprintCommitHash": "blueprint_commit_hash",
            "name": "name",
            "status": "status",
        }

        if order_by:
            order_by = parse_order_by(order_by, mapping, "id")

        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )

        query = "SELECT " + BuildScheme.db_fields() + " FROM $table " + where_query
        rows = tx.with_table_scope(builds_table).select(query, *where_args, model=BuildScheme)

        response = BuildList()

        for build in rows:
            response.builds.append(build.to_public())

        if limit is not None and len(response.builds) == limit:
            response.next_page_token = response.builds[-1].id

        return response

    @classmethod
    @mkt_transaction()
    def create(cls, blueprint_id, mkt_task_id, blueprint_commit_hash, build_id=None, rest=None, *, tx) -> BuildScheme:
        build = BuildScheme.new(
            build_id=build_id if build_id else None,
            blueprint_id=blueprint_id,
            mkt_task_id=mkt_task_id,
            blueprint_commit_hash=blueprint_commit_hash,
        )
        build.validate()
        tx.with_table_scope(builds_table).insert_object("INSERT INTO $table", build)

        return build

    @classmethod
    @mkt_transaction()
    def rpc_start(cls, build_id, *, tx) -> BuildOperation:
        build = cls.get(build_id, tx=tx)

        build.updated_at = timestamp()
        build.status = BuildStatus.RUNNING

        tx.with_table_scope(builds_table).insert_object("UPSERT INTO $table", build)

        op = BuildOperation({
            "id": generate_id(),
            "description": "build_start",
            "created_at": timestamp(),
            "done": True,
            "metadata": BuildMetadata({"build_id": build_id}).to_primitive(),
            "response": build.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_build_start", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_finish(cls, build_id, compute_image_id, status, *, tx) -> BuildOperation:
        build = cls.get(build_id, tx=tx)

        build.updated_at = timestamp()
        build.status = status
        build.compute_image_id = compute_image_id

        tx.with_table_scope(builds_table).insert_object("UPSERT INTO $table", build)

        op = BuildOperation({
            "id": generate_id(),
            "description": "build_finish",
            "created_at": timestamp(),
            "done": True,
            "metadata": BuildMetadata({"build_id": build_id}).to_primitive(),
            "response": build.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_build_finish", tx=tx)
