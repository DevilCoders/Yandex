from fastapi import Depends, status, APIRouter
from sqlalchemy.orm import Session, joinedload

import database
import database.models
import database.utils
from api import models, utils
from api.dependencies import db, api_key

router = APIRouter(
    prefix="/recipes",
    tags=["recipes"],
)


@router.get("", response_model=models.ClusterRecipeListResponse)
def list_recipes(db: Session = Depends(db)):
    recipes = (
        db.query(database.models.ClusterRecipe)
            .options(joinedload(database.models.ClusterRecipe.files))
            .all()
    )
    return models.ClusterRecipeListResponse(
        recipes=[models.ClusterRecipe.from_orm(recipe) for recipe in recipes]
    )


@router.post("", response_model=models.CreateClusterRecipeResponse, status_code=status.HTTP_201_CREATED)
def create_recipe(
    request: models.CreateClusterRecipeRequest,
    db: Session = Depends(db),
    _: str = Depends(api_key)
) -> models.CreateClusterRecipeResponse:
    recipe = database.models.ClusterRecipe(
        manually_created=request.manually_created,
        arcadia_path=request.arcadia_path,
        name=request.name,
        description=request.description,
    )
    db.add(recipe)
    db.commit()

    return models.CreateClusterRecipeResponse(
        recipe=models.ClusterRecipe.from_orm(recipe)
    )


@router.post(
    "/copy",
    response_model=models.CreateClusterRecipeResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Requested recipe not found",
        },
    }
)
def copy_recipe(
    request: models.CopyClusterRecipeRequest,
    db: Session = Depends(db),
    _: str = Depends(api_key),
) -> models.CreateClusterRecipeResponse:
    recipe = utils.get_recipe_or_404(db, request.recipe_id)
    copied_recipe = database.utils.copy_object(recipe)
    db.add(copied_recipe)
    db.commit()

    for file in recipe.files:
        copied_file = database.utils.copy_object(file)
        copied_file.recipe_id = copied_recipe.id
        db.add(copied_file)
    db.commit()

    copied_recipe = utils.get_recipe_or_404(db, copied_recipe.id)

    return models.CreateClusterRecipeResponse(
        recipe=models.ClusterRecipe.from_orm(copied_recipe)
    )


@router.get(
    "/{recipe_id}",
    response_model=models.ClusterRecipeResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found",
        }
    }
)
def get_recipe(recipe_id: int, db: Session = Depends(db)) -> models.ClusterRecipeResponse:
    recipe = utils.get_recipe_or_404(db, recipe_id)

    return models.ClusterRecipeResponse(
        recipe=models.ClusterRecipe.from_orm(recipe)
    )


@router.put(
    "/{recipe_id}",
    response_model=models.UpdateClusterRecipeResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found",
        }
    }
)
def update_recipe(
    recipe_id: int,
    request: models.UpdateClusterRecipeRequest,
    db: Session = Depends(db),
    _: str = Depends(api_key)
) -> models.UpdateClusterRecipeResponse:
    recipe = utils.get_recipe_or_404(db, recipe_id)

    recipe.name = request.name
    recipe.description = request.description
    recipe.manually_created = request.manually_created
    recipe.arcadia_path = request.arcadia_path
    db.commit()

    return models.UpdateClusterRecipeResponse(
        recipe=models.ClusterRecipe.from_orm(recipe)
    )


@router.delete(
    "/{recipe_id}",
    response_model=models.SuccessResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Recipe not found",
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Some clusters are built from this recipe. Delete clusters first",
        },
    }
)
def delete_recipe(
    recipe_id: int,
    db: Session = Depends(db),
    _: str = Depends(api_key)
) -> models.SuccessResponse:
    recipe = utils.get_recipe_or_404(db, recipe_id)
    clusters = list(
        db.query(database.models.Cluster).filter(
            database.models.Cluster.recipe_id == recipe_id
        )
    )
    utils.validate_request(
        len(clusters) == 0,
        error_message="Some clusters are built from this recipe. "
                      f"Delete clusters {[cluster.name for cluster in clusters]!r} first",
        status_code=status.HTTP_412_PRECONDITION_FAILED
    )

    db.delete(recipe)
    db.commit()

    return models.SuccessResponse()
