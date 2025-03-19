from fastapi import Depends, BackgroundTasks, APIRouter
from sqlalchemy.orm import Session, joinedload
from starlette import status

import database.models
import database.utils
from api import models, utils
from api.agents import update_prober_in_s3, update_all_clusters_in_s3, uploader, AgentConfigS3Uploader
from api.dependencies import db, api_key

router = APIRouter(
    prefix="/probers",
    tags=["probers"],
)


@router.get("", response_model=models.ProberListResponse, response_model_exclude_none=True)
def list_probers(db: Session = Depends(db)) -> models.ProberListResponse:
    probers = db.query(database.models.Prober).options(
        joinedload(database.models.Prober.configs),
        joinedload(database.models.Prober.files),
        joinedload(database.models.Prober.runner),
    ).all()
    return models.ProberListResponse(
        probers=[models.Prober.from_orm(prober) for prober in probers]
    )


@router.post(
    "",
    response_model=models.CreateProberResponse,
    response_model_exclude_none=True,
    status_code=status.HTTP_201_CREATED,
)
def create_prober(
    request: models.CreateProberRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.CreateProberResponse:
    prober = database.models.Prober(
        name=request.name,
        slug=request.slug,
        description=request.description,
        manually_created=request.manually_created,
        arcadia_path=request.arcadia_path,
        runner=request.runner.create_database_object(),
    )
    db.add(prober)
    db.commit()

    prober = utils.get_prober_or_404(db, prober.id)
    background_tasks.add_task(update_prober_in_s3, uploader, prober)

    return models.CreateProberResponse(
        prober=models.Prober.from_orm(prober)
    )


@router.post(
    "/copy",
    response_model=models.CreateProberResponse,
    response_model_exclude_none=True,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Requested prober not found",
        },
    }
)
def copy_prober(
    request: models.CopyProberRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.CreateProberResponse:
    prober = utils.get_prober_or_404(
        db,
        request.prober_id,
        status_code=status.HTTP_412_PRECONDITION_FAILED
    )
    copied_prober = database.utils.copy_object(prober)
    db.add(copied_prober)
    copied_prober.runner = database.utils.copy_object(prober.runner)
    db.commit()

    for file in prober.files:
        copied_file = database.utils.copy_object(file)
        copied_file.prober_id = copied_prober.id
        db.add(copied_file)
    db.commit()

    copied_prober = utils.get_prober_or_404(db, copied_prober.id)
    background_tasks.add_task(update_prober_in_s3, uploader, copied_prober)

    return models.CreateProberResponse(
        prober=models.Prober.from_orm(copied_prober)
    )


@router.get(
    "/{prober_id}",
    response_model=models.ProberResponse,
    response_model_exclude_none=True,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found",
        }
    }
)
def get_prober(prober_id: int, db: Session = Depends(db)) -> models.ProberResponse:
    prober = utils.get_prober_or_404(db, prober_id)
    return models.ProberResponse(
        prober=models.Prober.from_orm(prober)
    )


@router.put(
    "/{prober_id}",
    response_model=models.UpdateProberResponse,
    response_model_exclude_none=True,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found",
        }
    }
)
def update_prober(
    prober_id: int,
    request: models.UpdateProberRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateProberResponse:
    prober = utils.get_prober_or_404(db, prober_id)

    prober.name = request.name
    prober.slug = request.slug
    prober.description = request.description
    prober.runner = request.runner.create_database_object()
    prober.manually_created = request.manually_created
    prober.arcadia_path = request.arcadia_path

    db.commit()

    prober = utils.get_prober_or_404(db, prober_id)
    background_tasks.add_task(update_prober_in_s3, uploader, prober)

    return models.UpdateProberResponse(
        prober=models.Prober.from_orm(prober)
    )


@router.delete(
    "/{prober_id}",
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found",
        }
    }
)
def delete_prober(
    prober_id: int,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.DeleteProberResponse:
    prober = utils.get_prober_or_404(db, prober_id)
    for config in prober.configs:
        db.delete(config)
    db.delete(prober)
    db.commit()

    background_tasks.add_task(update_all_clusters_in_s3, db, uploader)

    return models.DeleteProberResponse()
