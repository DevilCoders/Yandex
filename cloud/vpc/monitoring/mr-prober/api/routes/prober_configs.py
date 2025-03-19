from typing import Union

from fastapi import Depends, BackgroundTasks, APIRouter
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session
from starlette import status
from starlette.exceptions import HTTPException

import database.models as db_models
import database.utils
import settings
from api import models, utils
from api.agents import update_all_clusters_in_s3, uploader, AgentConfigS3Uploader
from api.dependencies import db, api_key

router = APIRouter(
    prefix="/probers/{prober_id}/configs",
    tags=["prober_configs"],
)


def validate_requested_timeout_and_interval(
    request: Union[models.CreateProberConfigRequest, models.UpdateProberConfigRequest]
):
    if request.interval_seconds is not None:
        utils.validate_request(
            request.interval_seconds >= 0, error_message="interval_seconds can not be less than zero"
        )
    if request.timeout_seconds is not None:
        utils.validate_request(request.timeout_seconds >= 0, error_message="timeout_seconds can not be less than zero")
    if request.timeout_seconds is not None and request.interval_seconds is not None:
        utils.validate_request(
            request.timeout_seconds <= request.interval_seconds,
            "timeout_seconds should be less or equal to interval_seconds"
        )


def validate_prober_config(db: Session, config: db_models.ProberConfig):
    if settings.is_sqlite:
        # SQLite has non-usual behavior in case of NULLs insertion into unique index.
        # It allows to have several NULL rows, and this doesn't break the constraint.
        # Duplicated configs (with same cluster_id and empty hosts_re)
        # is not a big problem for us, but let's check for user's safety.
        count = db.query(db_models.ProberConfig).filter(
            db_models.ProberConfig.prober_id == config.prober_id,
            db_models.ProberConfig.cluster_id == config.cluster_id,
            db_models.ProberConfig.hosts_re == config.hosts_re
        ).count()
        utils.validate_request(
            count == 0,
            error_message="Same config (with same cluster_id and hosts_re) already exists for this prober. "
                          "Try to delete or edit it.",
            status_code=status.HTTP_412_PRECONDITION_FAILED,
        )


@router.post(
    "",
    response_model=models.CreateProberConfigResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Bad values in request: i.e. negative timeout_seconds"
        }
    }
)
def create_prober_config(
    prober_id: int,
    request: models.CreateProberConfigRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key),
) -> models.CreateProberConfigResponse:
    utils.get_prober_or_404(db, prober_id)

    validate_requested_timeout_and_interval(request)

    if request.cluster_id is not None:
        utils.get_cluster_or_404(db, request.cluster_id, status_code=status.HTTP_412_PRECONDITION_FAILED)

    config = db_models.ProberConfig(
        prober_id=prober_id,
        manually_created=request.manually_created,
        cluster_id=request.cluster_id,
        hosts_re=request.hosts_re,

        interval_seconds=request.interval_seconds,
        timeout_seconds=request.timeout_seconds,
        is_prober_enabled=request.is_prober_enabled,

        s3_logs_policy=request.s3_logs_policy,
        default_routing_interface=request.default_routing_interface,
        dns_resolving_interface=request.dns_resolving_interface,
    )
    if request.variables:
        config.variables = [
            db_models.ProberVariable(name=name, value=value) for name, value in request.variables.items()
        ]
    if request.matrix_variables:
        config.matrix_variables = [
            db_models.ProberMatrixVariable(name=name, values=values) for name, values in request.matrix_variables.items()
        ]

    validate_prober_config(db, config)
    db.add(config)
    try:
        db.commit()
    except IntegrityError as e:
        raise HTTPException(
            status_code=status.HTTP_412_PRECONDITION_FAILED,
            detail=f"Can't create config: {e}"
        )

    background_tasks.add_task(update_all_clusters_in_s3, db, uploader)

    prober = utils.get_prober_or_404(db, prober_id)
    config = utils.get_prober_config_or_404(db, prober.id, config.id)

    return models.CreateProberConfigResponse(
        prober=models.Prober.from_orm(prober),
        config=models.ProberConfig.from_orm(config)
    )


@router.post(
    "/copy",
    response_model=models.ProberResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found",
        },
        status.HTTP_400_BAD_REQUEST: {
            "model": models.ErrorResponse,
            "description": "Can not copy configs from the prober to itself",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Specified prober for copying not found",
        }
    }
)
def copy_prober_configs(
    prober_id: int,
    request: models.CopyOrMoveProberConfigsRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key),
) -> models.ProberResponse:
    utils.get_prober_or_404(db, prober_id)
    copied_prober = utils.get_prober_or_404(db, request.prober_id, status_code=status.HTTP_412_PRECONDITION_FAILED)
    utils.validate_request(
        prober_id != request.prober_id,
        error_message="Can not copy configs from the prober to itself",
    )

    for config in copied_prober.configs:
        copied_config = database.utils.copy_object(config, omit_foreign_keys=False)
        copied_config.prober_id = prober_id

        # Copy variables as well as matrix variables
        copied_config.variables = [database.utils.copy_object(variable) for variable in config.variables]
        copied_config.matrix_variables = [database.utils.copy_object(variable) for variable in config.matrix_variables]

        validate_prober_config(db, copied_config)
        db.add(copied_config)

    try:
        db.commit()
    except IntegrityError as e:
        raise HTTPException(
            status_code=status.HTTP_412_PRECONDITION_FAILED,
            detail=f"Can't copy configs: {e}"
        )

    background_tasks.add_task(update_all_clusters_in_s3, db, uploader)

    prober = utils.get_prober_or_404(db, prober_id)

    return models.ProberResponse(
        prober=models.Prober.from_orm(prober),
    )


@router.post(
    "/move",
    response_model=models.ProberResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found",
        },
        status.HTTP_400_BAD_REQUEST: {
            "model": models.ErrorResponse,
            "description": "Can not move configs from the prober to itself",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Specified prober for moving not found",
        }
    }
)
def move_prober_configs(
    prober_id: int,
    request: models.CopyOrMoveProberConfigsRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key),
) -> models.ProberResponse:
    utils.get_prober_or_404(db, prober_id)
    moved_prober = utils.get_prober_or_404(db, request.prober_id, status_code=status.HTTP_412_PRECONDITION_FAILED)
    utils.validate_request(
        prober_id != request.prober_id,
        error_message="Can not move configs from the prober to itself",
    )

    for config in moved_prober.configs:
        config.prober_id = prober_id
        validate_prober_config(db, config)

    try:
        db.commit()
    except IntegrityError as e:
        raise HTTPException(
            status_code=status.HTTP_412_PRECONDITION_FAILED,
            detail=f"Can't move configs: {e}"
        )

    background_tasks.add_task(update_all_clusters_in_s3, db, uploader)

    prober = utils.get_prober_or_404(db, prober_id)

    return models.ProberResponse(
        prober=models.Prober.from_orm(prober),
    )


@router.delete(
    "/{config_id}",
    response_model=models.DeleteProberConfigResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Either prober or config not found",
        }
    }
)
def delete_prober_config(
    prober_id: int,
    config_id: int,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key),
) -> models.DeleteProberConfigResponse:
    config = utils.get_prober_config_or_404(db, prober_id, config_id)

    db.delete(config)
    db.commit()

    background_tasks.add_task(update_all_clusters_in_s3, db, uploader)

    return models.DeleteProberConfigResponse()


@router.put(
    "/{config_id}",
    response_model=models.UpdateProberConfigResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Either prober or config not found",
        }
    }
)
def update_prober_config(
    prober_id: int,
    config_id: int,
    request: models.UpdateProberConfigRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key),
) -> models.UpdateProberConfigResponse:
    config = utils.get_prober_config_or_404(db, prober_id, config_id)

    validate_requested_timeout_and_interval(request)

    config.manually_created = request.manually_created
    config.is_prober_enabled = request.is_prober_enabled
    config.interval_seconds = request.interval_seconds
    config.timeout_seconds = request.timeout_seconds
    config.s3_logs_policy = request.s3_logs_policy
    config.default_routing_interface = request.default_routing_interface
    config.dns_resolving_interface = request.dns_resolving_interface
    config.cluster_id = request.cluster_id
    config.hosts_re = request.hosts_re

    config.variables.clear()
    config.matrix_variables.clear()
    db.flush()

    if request.variables:
        config.variables = [
            db_models.ProberVariable(name=name, value=value) for name, value in request.variables.items()
        ]
    if request.matrix_variables:
        config.matrix_variables = [
            db_models.ProberMatrixVariable(name=name, values=values) for name, values in request.matrix_variables.items()
        ]
    db.commit()

    background_tasks.add_task(update_all_clusters_in_s3, db, uploader)

    prober = utils.get_prober_or_404(db, prober_id)
    config = utils.get_prober_config_or_404(db, prober.id, config.id)

    return models.UpdateProberConfigResponse(
        prober=models.Prober.from_orm(prober),
        config=models.ProberConfig.from_orm(config)
    )
