import mimetypes

from fastapi import Depends, HTTPException, status, File, Response, APIRouter
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session

import database
import database.models
from api import models, utils
from api.dependencies import db, api_key

router = APIRouter(
    prefix="/recipes/{recipe_id}/files",
    tags=["recipe_files"],
)


def prohibit_files_modification_for_working_recipes(db: Session, recipe_id: int):
    count = db.query(database.models.Cluster).filter(
        database.models.Cluster.recipe_id == recipe_id
    ).count()
    utils.validate_request(
        count == 0,
        error_message="It's prohibited to update files for recipes which have at least one cluster built from it. "
                      "You can create a new recipe as a copy of this one, edit files and move cluster to "
                      "new recipe. \n\n"
                      "Alternatively, you can add ?force=true or {..., \"force\": true} to the request "
                      "if you're sure what you do.\n\n"
                      "Why files updating is prohibited?\n"
                      "Because in common cases you want to update several files correspondingly and atomically, but "
                      "creator can download one file without edits to another one. So we allow edit files only for recipes "
                      "without any cluster built from it.",
        status_code=status.HTTP_412_PRECONDITION_FAILED
    )


@router.post(
    "",
    response_model=models.CreateClusterRecipeFileResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found",
        },
        status.HTTP_409_CONFLICT: {
            "model": models.ErrorResponse,
            "description": "File with same path already exists for this recipe"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to update files for recipes which have at least one cluster built from it",
        },
    }
)
def add_recipe_file(
    recipe_id: int,
    request: models.CreateClusterRecipeFileRequest,
    db: Session = Depends(db),
    _: str = Depends(api_key)
) -> models.CreateClusterRecipeFileResponse:
    utils.get_recipe_or_404(db, recipe_id, status_code=status.HTTP_412_PRECONDITION_FAILED)
    if not request.force:
        prohibit_files_modification_for_working_recipes(db, recipe_id)

    file = database.models.ClusterRecipeFile(
        recipe_id=recipe_id,
        relative_file_path=request.relative_file_path
    )
    db.add(file)
    try:
        db.commit()
    except IntegrityError:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"File with path {file.relative_file_path} already exists for recipe {recipe_id}"
        )

    recipe = utils.get_recipe_or_404(db, recipe_id)

    return models.CreateClusterRecipeFileResponse(
        recipe=models.ClusterRecipe.from_orm(recipe),
        file=models.ClusterRecipeFile.from_orm(file)
    )


@router.get(
    "/{file_id}",
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found or file not found",
        }
    }
)
def update_recipe_file(recipe_id: int, file_id: int, db: Session = Depends(db)) -> models.ClusterRecipeFileResponse:
    file = utils.get_file_or_404(db, recipe_id, file_id)

    return models.ClusterRecipeFileResponse(
        file=models.ClusterRecipeFile.from_orm(file),
    )


@router.put(
    "/{file_id}",
    response_model=models.UpdateClusterRecipeFileResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found or file not found",
        },
        status.HTTP_409_CONFLICT: {
            "model": models.ErrorResponse,
            "description": "File with same path already exists for this recipe"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to update files for recipes which have at least one cluster built from it",
        },
    }
)
def update_recipe_file(
    recipe_id: int,
    file_id: int,
    request: models.UpdateClusterRecipeFileRequest,
    db: Session = Depends(db),
    _: str = Depends(api_key)
) -> models.UpdateClusterRecipeFileResponse:
    file = utils.get_file_or_404(db, recipe_id, file_id)

    if not request.force:
        prohibit_files_modification_for_working_recipes(db, recipe_id)

    file.relative_file_path = request.relative_file_path
    try:
        db.commit()
    except IntegrityError:
        db.rollback()
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"File with path {request.relative_file_path} already exists for recipe {recipe_id}"
        )

    recipe = utils.get_recipe_or_404(db, recipe_id)

    return models.UpdateClusterRecipeFileResponse(
        recipe=models.ClusterRecipe.from_orm(recipe),
        file=models.ClusterRecipeFile.from_orm(file)
    )


@router.delete(
    "/{file_id}",
    response_model=models.ClusterRecipeResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found or file not found",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to update files for recipes which have at least one cluster built from it",
        },
    }
)
def delete_recipe_file(
    recipe_id: int,
    file_id: int,
    force: bool = False,
    db: Session = Depends(db),
    _: str = Depends(api_key)
) -> models.ClusterRecipeResponse:
    file = utils.get_file_or_404(db, recipe_id, file_id)

    if not force:
        prohibit_files_modification_for_working_recipes(db, recipe_id)

    db.delete(file)
    db.commit()

    recipe = utils.get_recipe_or_404(db, recipe_id)

    return models.ClusterRecipeResponse(
        recipe=models.ClusterRecipe.from_orm(recipe),
    )


@router.get(
    "/{file_id}/content",
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found, file not found or content hasn't been uploaded yet",
        }
    }
)
def download_recipe_file(recipe_id: int, file_id: int, db: Session = Depends(db)) -> Response:
    file = utils.get_file_or_404(db, recipe_id, file_id)
    content_type, _ = mimetypes.guess_type(file.relative_file_path)

    if file.content is None:
        # File is not uploaded yet, return 404
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"File hasn't been uploaded yet. Upload it via PUT /recipes/{recipe_id}/files/{file_id}/content"
        )

    return Response(content=file.content, media_type=content_type)


@router.put(
    "/{file_id}/content",
    response_model=models.UpdateClusterRecipeFileResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found or file not found",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "It's prohibited to update files for recipes which have at least one cluster built from it",
        },
    }
)
def upload_recipe_file(
    recipe_id: int,
    file_id: int,
    force: bool = False,
    content: bytes = File(...),
    db: Session = Depends(db),
    _: str = Depends(api_key)
) -> models.UpdateClusterRecipeFileResponse:
    file = utils.get_file_or_404(db, recipe_id, file_id)

    if not force:
        prohibit_files_modification_for_working_recipes(db, recipe_id)

    file.content = content
    db.commit()

    recipe = utils.get_recipe_or_404(db, recipe_id)

    return models.UpdateClusterRecipeFileResponse(
        recipe=models.ClusterRecipe.from_orm(recipe),
        file=models.ClusterRecipeFile.from_orm(file)
    )
