import hashlib
import io
from typing import List, TypeVar, Type

from fastapi import HTTPException, status
from sqlalchemy.orm import Session, joinedload

import database.models


def validate_request(
    condition: bool, error_message: str = "Invalid request", status_code: int = status.HTTP_412_PRECONDITION_FAILED,
):
    """
    Simple helper for validation requests.

    Example of usage:
    utils.validate_request(request.interval_seconds < 0, error_message="interval_seconds can not be less than zero")
    """
    if not condition:
        raise HTTPException(
            status_code,
            detail=error_message
        )


Model = TypeVar("Model", bound=database.models.Base)


def get_object_or_404(
    db: Session, model: Type[Model], primary_key: any,
    error_message: str = "Not found",
    options: List[any] = None,
    status_code: int = status.HTTP_404_NOT_FOUND
) -> Model:
    """
    Loads object from database by it's Primary Key or returns 404.

    Example of usage:
    recipe = utils.get_object_or_404(
        db, database.models.ClusterRecipe, recipe_id,
        error_message=f"Recipe {recipe_id} not found",
        options=[joinedload(database.models.ClusterRecipe.files)],
    )
    """
    if options is None:
        options = []

    query = db.query(model)

    for option in options:
        query = query.options(option)

    obj = query.get(primary_key)
    validate_request(obj is not None, error_message=error_message, status_code=status_code)

    return obj


def get_cluster_or_404(
    db: Session, cluster_id: int, status_code: int = status.HTTP_404_NOT_FOUND
) -> database.models.Cluster:
    return get_object_or_404(
        db, database.models.Cluster, cluster_id,
        error_message=f"Cluster {cluster_id} not found",
        options=[
            joinedload(database.models.Cluster.recipe).options(joinedload(database.models.ClusterRecipe.files)),
            joinedload(database.models.Cluster.variables),
        ],
        status_code=status_code,
    )


def get_cluster_variable_or_404(db: Session, cluster_id: int, variable_id: int) -> database.models.ClusterVariable:
    variable = get_object_or_404(
        db, database.models.ClusterVariable, variable_id,
        error_message=f"Variable {variable_id} not found"
    )

    validate_request(
        variable.cluster_id == cluster_id,
        error_message=f"Variable {variable_id} doesn't belong to cluster {cluster_id}",
        status_code=status.HTTP_404_NOT_FOUND,
    )

    return variable


def get_recipe_or_404(
    db: Session, recipe_id: int, status_code: int = status.HTTP_404_NOT_FOUND
) -> database.models.ClusterRecipe:
    return get_object_or_404(
        db, database.models.ClusterRecipe, recipe_id,
        error_message=f"Recipe {recipe_id} not found",
        options=[joinedload(database.models.ClusterRecipe.files)],
        status_code=status_code,
    )


def get_file_or_404(
    db: Session, recipe_id: int, file_id: int, status_code: int = status.HTTP_404_NOT_FOUND
) -> database.models.ClusterRecipeFile:
    file = get_object_or_404(
        db, database.models.ClusterRecipeFile, file_id,
        error_message=f"File {recipe_id} not found",
        status_code=status_code,
    )

    validate_request(
        file.recipe_id == recipe_id,
        error_message=f"File {file_id} doesn't belong to recipe {recipe_id}",
        status_code=status.HTTP_404_NOT_FOUND,
    )

    return file


def get_prober_or_404(
    db: Session, prober_id: int, status_code: int = status.HTTP_404_NOT_FOUND
) -> database.models.Prober:
    return get_object_or_404(
        db, database.models.Prober, prober_id,
        error_message=f"Prober {prober_id} not found",
        options=[
            joinedload(database.models.Prober.configs),
            joinedload(database.models.Prober.files),
            joinedload(database.models.Prober.runner),
        ],
        status_code=status_code,
    )


def get_prober_config_or_404(db: Session, prober_id: int, config_id: int) -> database.models.ProberConfig:
    config = get_object_or_404(
        db, database.models.ProberConfig, config_id,
        error_message=f"Prober config {config_id} not found",
    )

    validate_request(
        config.prober_id == prober_id,
        error_message=f"Config {config_id} doesn't belong to prober {prober_id}",
        status_code=status.HTTP_404_NOT_FOUND,
    )

    return config


def get_prober_file_or_404(
    db: Session, prober_id: int, file_id: int, status_code: int = status.HTTP_404_NOT_FOUND
) -> database.models.ProberFile:
    file = get_object_or_404(
        db, database.models.ProberFile, file_id,
        error_message=f"File {file_id} not found",
        status_code=status_code,
    )

    validate_request(
        file.prober_id == prober_id,
        error_message=f"File {file_id} doesn't belong to prober {prober_id}",
        status_code=status_code,
    )

    return file


def md5(file: (bytes, io.FileIO)) -> str:
    if isinstance(file, bytes):
        return hashlib.md5(file).hexdigest()
    else:
        hash_md5 = hashlib.md5()
        for chunk in iter(lambda: file.read(4096), b""):
            hash_md5.update(chunk)
        return hash_md5.hexdigest()
