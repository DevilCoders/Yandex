from typing import Optional

from fastapi import Depends, status, BackgroundTasks, APIRouter
from fastapi_cache import FastAPICache
from sqlalchemy.orm import Session, joinedload
from starlette.requests import Request
from starlette.responses import Response

import database
import database.models
from api import models, utils
from api.agents import update_cluster_in_s3, uploader
from api.dependencies import db, api_key
from agent.config.s3.uploader import AgentConfigS3Uploader
from fastapi_cache.decorator import cache

router = APIRouter(
    prefix="/clusters",
    tags=["clusters"],
)


def key_builder_without_arguments(
    func,
    namespace: Optional[str] = "",
    request: Optional[Request] = None,
    response: Optional[Response] = None,
    *args,
    **kwargs,
):
    prefix = FastAPICache.get_prefix()
    cache_key = f"{prefix}:{namespace}:{func.__module__}:{func.__name__}"
    return cache_key


@router.get("", response_model=models.ClusterListResponse)
# CLOUD-97970. 'yc-e2e-permnet status' makes a lot of queries to /clusters, but it's almost never changed,
# so let's cache it for some time to decrease database load.
@cache(expire=5, key_builder=key_builder_without_arguments)
async def list_clusters(db: Session = Depends(db)) -> models.ClusterListResponse:
    clusters = db.query(
        database.models.Cluster
    ).options(
        joinedload(database.models.Cluster.recipe).options(joinedload(database.models.ClusterRecipe.files)),
        joinedload(database.models.Cluster.variables),
        joinedload(database.models.Cluster.deploy_policy)
    ).all()
    return models.ClusterListResponse(
        clusters=[models.Cluster.from_orm(cluster) for cluster in clusters]
    )


@router.get(
    "/{cluster_id}",
    response_model=models.ClusterResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster not found",
        }
    }
)
def get_cluster(cluster_id: int, db: Session = Depends(db)) -> models.ClusterResponse:
    cluster = utils.get_cluster_or_404(db, cluster_id)
    return models.ClusterResponse(
        cluster=models.Cluster.from_orm(cluster)
    )


@router.post(
    "",
    response_model=models.CreateClusterResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Recipe with specified id not found",
        }
    }
)
def create_cluster(
    request: models.CreateClusterRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key),
) -> models.CreateClusterResponse:
    utils.get_recipe_or_404(db, request.recipe_id, status_code=status.HTTP_412_PRECONDITION_FAILED)

    cluster = database.models.Cluster(
        recipe_id=request.recipe_id,
        name=request.name,
        slug=request.slug,
        manually_created=request.manually_created,
        arcadia_path=request.arcadia_path,
        variables=[database.models.ClusterVariable(name=name, value=value) for name, value in
                   request.variables.items()],
        deploy_policy=request.deploy_policy.create_database_object() if request.deploy_policy is not None else None,
    )
    db.add(cluster)
    db.commit()

    cluster = utils.get_cluster_or_404(db, cluster.id)

    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.CreateClusterResponse(
        cluster=models.Cluster.from_orm(cluster)
    )


@router.put(
    "/{cluster_id}",
    response_model=models.UpdateClusterResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster not found",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Recipe with specified id not found",
        }
    }
)
def update_cluster(
    cluster_id: int,
    request: models.UpdateClusterRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterResponse:
    cluster = utils.get_cluster_or_404(db, cluster_id)

    utils.get_recipe_or_404(db, request.recipe_id, status_code=status.HTTP_412_PRECONDITION_FAILED)

    # We have to remove old dependent objects and flush the session:
    # https://github.com/sqlalchemy/sqlalchemy/issues/2501
    cluster.variables.clear()
    cluster.deploy_policy = None
    db.flush()

    cluster.manually_created = request.manually_created
    cluster.arcadia_path = request.arcadia_path
    cluster.recipe_id = request.recipe_id
    cluster.name = request.name
    cluster.slug = request.slug
    cluster.variables = [database.models.ClusterVariable(name=name, value=value) for name, value in
                         request.variables.items()]
    cluster.deploy_policy = request.deploy_policy.create_database_object() if request.deploy_policy is not None else None
    db.commit()

    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)
    cluster = utils.get_cluster_or_404(db, cluster_id)
    return models.UpdateClusterResponse(
        cluster=models.Cluster.from_orm(cluster)
    )
