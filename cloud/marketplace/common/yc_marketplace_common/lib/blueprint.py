from typing import Optional

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import blueprints_table
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import Blueprint as BlueprintScheme
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintList
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintMetadata
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintOperation
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintStatus
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.utils.errors import BlueprintBuildLinksNotFound
from cloud.marketplace.common.yc_marketplace_common.utils.errors import BlueprintInvalidStatus
from cloud.marketplace.common.yc_marketplace_common.utils.errors import BlueprintNotFoundError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import BlueprintTestLinksNotFound
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from yc_common import logging
from yc_common.misc import drop_none
from yc_common.misc import timestamp
from yc_common.paging import page_handler
from yc_common.validation import ResourceIdType


log = logging.get_logger('yc')


class Blueprint:
    @staticmethod
    @mkt_transaction()
    def get(blueprint_id: ResourceIdType, *, tx) -> BlueprintScheme:
        """Internal snake_case get"""
        blueprint = tx.with_table_scope(blueprints_table).select_one(
            "SELECT " + BlueprintScheme.db_fields() + " " +
            "FROM $table "
            "WHERE id = ?", blueprint_id,
            model=BlueprintScheme)
        if blueprint is None:
            raise BlueprintNotFoundError()

        return blueprint

    @classmethod
    @mkt_transaction()
    def rpc_get(cls, id, *, tx):
        """Get for API requests"""
        return cls.get(id, tx=tx).to_public()

    @staticmethod
    @mkt_transaction()
    @page_handler(items="blueprints")
    def rpc_list(cursor: Optional[str],
                 limit: Optional[int] = 100,
                 *,
                 publisher_account_id=None,
                 order_by: Optional[str] = None,
                 filter_query: Optional[str] = None,
                 tx=None) -> BlueprintList:
        filter_query, filter_args = parse_filter(filter_query, BlueprintScheme)

        if publisher_account_id:
            filter_query += ["publisher_account_id = ?"]
            filter_args += [publisher_account_id]

        filter_query = " AND ".join(filter_query)

        mapping = {
            "id": "id",
            "publisherAccountId": "publisher_account_id",
            "commitHash": "commit_hash",
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

        query = "SELECT " + BlueprintScheme.db_fields() + " FROM $table " + where_query
        rows = tx.with_table_scope(blueprints_table).select(query, *where_args, model=BlueprintScheme)

        response = BlueprintList()

        for blueprint in rows:
            response.blueprints.append(blueprint.to_public())

        if limit is not None and len(response.blueprints) == limit:
            response.next_page_token = response.blueprints[-1].id

        return response

    @classmethod
    @mkt_transaction()
    def rpc_create(cls, request: BlueprintCreateRequest, *, tx) -> BlueprintOperation:
        blueprint = request.to_model()
        blueprint.validate()

        tx.with_table_scope(blueprints_table).insert_object("INSERT INTO $table", blueprint)
        op = BlueprintOperation({
            "id": blueprint.id,
            "description": "blueprint_create",
            "created_at": timestamp(),
            "done": True,
            "metadata": BlueprintMetadata({"blueprint_id": blueprint.id}).to_primitive(),
            "response": blueprint.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_blueprint_create", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_update(cls, request: BlueprintUpdateRequest, *, tx) -> BlueprintOperation:
        blueprint = cls.get(request.blueprint_id, tx=tx)

        blueprint_update = drop_none({
            "name": request.name,
            "build_recipe_links": request.build_recipe_links,
            "test_suites_links": request.test_suites_links,
            "test_instance_config": request.test_instance_config,
            "commit_hash": request.commit_hash,
        })

        blueprint.updated_at = timestamp()
        if blueprint_update:
            for k in blueprint_update:
                setattr(blueprint, k, blueprint_update[k])
            tx.with_table_scope(blueprints_table).insert_object("UPSERT INTO $table", blueprint)

        op = BlueprintOperation({
            "id": blueprint.id,
            "description": "blueprint_update",
            "created_at": timestamp(),
            "done": True,
            "metadata": BlueprintMetadata({"blueprint_id": blueprint.id}).to_primitive(),
            "response": blueprint.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_blueprint_update", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_accept(cls, blueprint_id, *, tx) -> BlueprintOperation:
        blueprint = cls.get(blueprint_id, tx=tx)

        blueprint.updated_at = timestamp()
        blueprint.status = BlueprintStatus.ACTIVE

        tx.with_table_scope(blueprints_table).insert_object("UPSERT INTO $table", blueprint)

        op = BlueprintOperation({
            "id": generate_id(),
            "description": "blueprint_accept",
            "created_at": timestamp(),
            "done": True,
            "metadata": BlueprintMetadata({"blueprint_id": blueprint_id}).to_primitive(),
            "response": blueprint.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_blueprint_accept", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_reject(cls, blueprint_id, *, tx) -> BlueprintOperation:
        blueprint = cls.get(blueprint_id, tx=tx)

        blueprint.updated_at = timestamp()
        blueprint.status = BlueprintStatus.REJECTED

        tx.with_table_scope(blueprints_table).insert_object("UPSERT INTO $table", blueprint)

        op = BlueprintOperation({
            "id": generate_id(),
            "description": "blueprint_reject",
            "created_at": timestamp(),
            "done": True,
            "metadata": BlueprintMetadata({"blueprint_id": blueprint_id}).to_primitive(),
            "response": blueprint.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_blueprint_reject", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_build(cls, blueprint_id, *, tx) -> BlueprintOperation:
        blueprint = cls.get(blueprint_id, tx=tx)

        if blueprint.status != BlueprintStatus.ACTIVE:
            raise BlueprintInvalidStatus()

        if not blueprint.build_recipe_links:
            raise BlueprintBuildLinksNotFound()

        if not blueprint.test_suites_links:
            raise BlueprintTestLinksNotFound()

        build_id = generate_id()
        params = {
            "manifest": {
                "blueprint": {
                    "type": "packertests",
                    "packerDirectory": "packer-remote",
                    "packerResultPath": "packer-remote/manifest.json",
                    "buildRecipes": blueprint.build_recipe_links,
                    "testSuites": blueprint.test_suites_links,
                    "testInstanceConfig": blueprint.test_instance_config,
                },
                "build_id": build_id,
            }
        }

        log.info("Task params: %s", params["manifest"])

        mkt_build_task = lib.TaskUtils.create(
            operation_type="construct_tested_image",
            params=params,
            tx=tx,
            is_infinite=False,
            kind=Task.Kind.BUILD,
            metadata=BlueprintMetadata({"blueprint_id": blueprint_id, "build_id": build_id}).to_primitive(),
        )

        build = lib.Build.create(
            blueprint_id=blueprint_id,
            mkt_task_id=mkt_build_task.id,
            blueprint_commit_hash=blueprint.commit_hash,
            build_id=build_id,
        )
        log.info("Blueprint {} created build: {}".format(blueprint_id, build))

        return mkt_build_task.to_api(False)
