import mimetypes

from fastapi import Depends, HTTPException, status, File, Response, APIRouter
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session
from starlette.background import BackgroundTasks

import database
import database.models
from api import models, utils
from api.agents import uploader, AgentConfigS3Uploader, update_prober_in_s3, update_prober_file_in_s3
from api.dependencies import db, api_key
from api.utils import md5, validate_request

router = APIRouter(
    prefix="/probers/{prober_id}/files",
    tags=["prober_files"],
)


def prohibit_files_modification_for_working_probers(db: Session, prober_id: int):
    count = db.query(database.models.ProberConfig).filter(
        database.models.ProberConfig.prober_id == prober_id
    ).count()
    validate_request(
        count == 0,
        error_message="It's prohibited to update files for probers which have at least one config. "
                      "You can create a new prober as a copy of this one, edit files and move configs from this prober to "
                      "new one. Or you can delete all configs from this prober, edit files and return configs back.\n\n"
                      "Alternatively, you can add ?force=true or {..., \"force\": true} to the request "
                      "if you're sure what you do.\n\n"
                      "Why files updating is prohibited?\n"
                      "Because in common cases you want to update several files correspondingly and atomically, but "
                      "agent can download one file without edits to another one. So we allow edit files only for probers "
                      "without configs (in other words not attached to any cluster).",
        status_code=status.HTTP_412_PRECONDITION_FAILED
    )


@router.post(
    "",
    response_model=models.CreateProberFileResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_400_BAD_REQUEST: {
            "model": models.ErrorResponse,
            "description": "`..` not allowed in the relative file path"
        },
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found",
        },
        status.HTTP_409_CONFLICT: {
            "model": models.ErrorResponse,
            "description": "File with same path already exists for this prober"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to update files for probers which have at least one config.",
        },
    }
)
def add_prober_file(
    prober_id: int,
    request: models.CreateProberFileRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.CreateProberFileResponse:
    utils.get_prober_or_404(db, prober_id)
    if not request.force:
        prohibit_files_modification_for_working_probers(db, prober_id)

    validate_request(
        ".." not in request.relative_file_path,
        error_message="`..` not allowed in the relative file path",
        status_code=status.HTTP_400_BAD_REQUEST,
    )

    file = database.models.ProberFile(
        prober_id=prober_id,
        relative_file_path=request.relative_file_path,
        is_executable=request.is_executable
    )
    db.add(file)
    try:
        db.commit()
    except IntegrityError as e:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"File with path {file.relative_file_path} already exists for prober {prober_id}: {e}"
        )

    prober = utils.get_prober_or_404(db, prober_id)
    background_tasks.add_task(update_prober_in_s3, uploader, prober)

    return models.CreateProberFileResponse(
        prober=models.Prober.from_orm(prober),
        file=models.ProberFile.from_orm(file)
    )


@router.get(
    "/{file_id}",
    response_model=models.ProberFileResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "File not found",
        }
    }
)
def get_prober_file(prober_id: int, file_id: int, db: Session = Depends(db)) -> models.ProberFileResponse:
    file = utils.get_prober_file_or_404(db, prober_id, file_id)

    return models.ProberFileResponse(
        file=models.ProberFile.from_orm(file),
    )


@router.put(
    "/{file_id}",
    response_model=models.UpdateProberFileResponse,
    responses={
        status.HTTP_400_BAD_REQUEST: {
            "model": models.ErrorResponse,
            "description": "`..` not allowed in the relative file path"
        },
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found or file not found",
        },
        status.HTTP_409_CONFLICT: {
            "model": models.ErrorResponse,
            "description": "File with same path already exists for this recipe"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to update files for probers which have at least one config.",
        },
    }
)
def update_prober_file(
    prober_id: int,
    file_id: int,
    request: models.UpdateProberFileRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateProberFileResponse:
    file = utils.get_prober_file_or_404(db, prober_id, file_id)
    if not request.force:
        prohibit_files_modification_for_working_probers(db, prober_id)

    validate_request(
        ".." not in request.relative_file_path,
        error_message="`..` not allowed in the relative file path",
        status_code=status.HTTP_400_BAD_REQUEST,
    )

    file.relative_file_path = request.relative_file_path
    file.is_executable = request.is_executable
    try:
        db.commit()
    except IntegrityError as e:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"File with path {request.relative_file_path} already exists for prober {prober_id}: {e}"
        )

    prober = utils.get_prober_or_404(db, prober_id)

    background_tasks.add_task(update_prober_in_s3, uploader, prober)

    return models.UpdateProberFileResponse(
        prober=models.Prober.from_orm(prober),
        file=models.ProberFile.from_orm(file)
    )


@router.delete(
    "/{file_id}",
    response_model=models.ProberResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found or file not found",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to delete files for probers which have at least one config.",
        },
    }
)
def delete_prober_file(
    prober_id: int,
    file_id: int,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    force: bool = False,
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.ProberResponse:
    file = utils.get_prober_file_or_404(db, prober_id, file_id)
    if not force:
        prohibit_files_modification_for_working_probers(db, prober_id)

    db.delete(file)
    db.commit()

    prober = utils.get_prober_or_404(db, prober_id)
    background_tasks.add_task(update_prober_in_s3, uploader, prober)

    return models.ProberResponse(
        prober=models.Prober.from_orm(prober),
    )


@router.get(
    "/{file_id}/content",
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "File not found or content hasn't been uploaded yet",
        }
    }
)
def download_prober_file(prober_id: int, file_id: int, db: Session = Depends(db)) -> Response:
    file = utils.get_prober_file_or_404(db, prober_id, file_id)
    content_type, _ = mimetypes.guess_type(file.relative_file_path)

    if file.content is None:
        # File is not uploaded yet, return 404
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"File hasn't been uploaded yet. Upload it via PUT /probers/{prober_id}/files/{file_id}/content"
        )

    return Response(content=file.content, media_type=content_type)


@router.put(
    "/{file_id}/content",
    response_model=models.UpdateProberFileResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Prober not found or file not found",
        },
        status.HTTP_409_CONFLICT: {
            "model": models.ErrorResponse,
            "description": "File with same path already exists for this recipe"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to edit files for probers which have at least one config.",
        },
    }
)
def upload_prober_file(
    prober_id: int,
    file_id: int,
    background_tasks: BackgroundTasks,
    content: bytes = File(...),
    db: Session = Depends(db),
    force: bool = False,
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateProberFileResponse:
    file = utils.get_prober_file_or_404(db, prober_id, file_id)

    if not force:
        prohibit_files_modification_for_working_probers(db, prober_id)

    file.content = content
    file.md5_hexdigest = md5(content)

    db.commit()

    prober = utils.get_prober_or_404(db, prober_id)
    file = utils.get_prober_file_or_404(db, prober_id, file_id)

    background_tasks.add_task(update_prober_in_s3, uploader, prober)
    background_tasks.add_task(update_prober_file_in_s3, uploader, file)

    return models.UpdateProberFileResponse(
        prober=models.Prober.from_orm(prober),
        file=models.ProberFile.from_orm(file)
    )
