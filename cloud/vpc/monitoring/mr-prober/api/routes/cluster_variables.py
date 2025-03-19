import copy

from fastapi import Depends, HTTPException, status, APIRouter
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session
from starlette.background import BackgroundTasks

import database.models
from api.agents import uploader, update_cluster_in_s3
from api import models, utils
from api.dependencies import db, api_key
from agent.config.s3.uploader import AgentConfigS3Uploader

router = APIRouter(
    prefix="/clusters/{cluster_id}/variables",
    tags=["cluster_variables"],
)


@router.post(
    "",
    response_model=models.CreateClusterVariableResponse,
    status_code=status.HTTP_201_CREATED,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster not found",
        },
        status.HTTP_409_CONFLICT: {
            "model": models.ErrorResponse,
            "description": "Variable with same name already exists for this cluster"
        }
    }
)
def add_cluster_variable(
    cluster_id: int,
    request: models.CreateClusterVariableRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key),
) -> models.CreateClusterVariableResponse:
    utils.get_cluster_or_404(db, cluster_id)

    variable = database.models.ClusterVariable(
        cluster_id=cluster_id,
        name=request.name,
        value=request.value
    )

    db.add(variable)
    try:
        db.commit()
    except IntegrityError:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"Variable with name {variable.name} already exists for cluster {cluster_id}"
        )

    cluster = utils.get_cluster_or_404(db, cluster_id)
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable.id)
    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)
    return models.CreateClusterVariableResponse(
        cluster=models.Cluster.from_orm(cluster), variable=models.ClusterVariable.from_orm(variable)
    )


@router.delete(
    "/{variable_id}",
    response_model=models.SuccessResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster not found or variable not found",
        }
    }
)
def delete_cluster_variable(
    cluster_id: int,
    variable_id: int,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.SuccessResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)

    db.delete(variable)
    db.commit()

    cluster = utils.get_cluster_or_404(db, cluster_id)
    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.SuccessResponse()


@router.put(
    "/{variable_id}",
    response_model=models.UpdateClusterVariableResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster or variable not found"
        }
    }
)
def update_cluster_variable(
    cluster_id: int,
    variable_id: int,
    request: models.UpdateClusterVariableRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterVariableResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)

    variable.value = request.value
    db.commit()

    cluster = utils.get_cluster_or_404(db, cluster_id)
    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.UpdateClusterVariableResponse(variable=variable)


@router.put(
    "/{variable_id}/list",
    response_model=models.UpdateClusterVariableResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster or variable not found"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Can't add element to list, because current value is not a list"
        }
    }
)
def update_cluster_variable_add_element_to_list(
    cluster_id: int,
    variable_id: int,
    request: models.UpdateClusterVariableRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterVariableResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)
    utils.validate_request(
        type(variable.value) is list,
        f"Can't add element to {variable.name}, because current value is not a list: {variable.value!r}."
    )

    variable.value = variable.value + [request.value]
    db.commit()

    cluster = utils.get_cluster_or_404(db, cluster_id)
    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.UpdateClusterVariableResponse(variable=variable)


@router.delete(
    "/{variable_id}/list",
    response_model=models.UpdateClusterVariableResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster or variable not found"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Can't delete element from list, because current value is not a list, "
                           "index is out of range, or value has been changed by someone else"
        }
    }
)
def update_cluster_variable_delete_element_from_list(
    cluster_id: int,
    variable_id: int,
    request: models.UpdateClusterVariableDeleteFromListRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterVariableResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)
    utils.validate_request(
        type(variable.value) is list,
        f"Can't delete element from {variable.value}, because current value is not a list: {variable.value!r}"
    )

    utils.validate_request(
        0 <= request.index < len(variable.value),
        f"Index {request.index} is out of range, it should be between 0 and {len(variable.value) - 1}"
    )

    element = variable.value[request.index]
    utils.validate_request(
        element == request.value,
        "Can't update value, because variable has been changed by someone else: "
        f"{variable.name}[{request.index}] is {element!r}, not {request.value!r}"
    )

    # SQLAlchemy does not notice a change if we modify one of the elements in the list, or append/remove a value.
    # To make this work we should either make sure we assign a new list each time.
    saved_value = variable.value[:]
    saved_value.pop(request.index)
    variable.value = saved_value
    db.commit()

    cluster = utils.get_cluster_or_404(db, cluster_id)
    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.UpdateClusterVariableResponse(variable=variable)


@router.put(
    "/{variable_id}/set",
    response_model=models.UpdateClusterVariableResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster or variable not found"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Can't add element to set, because current value is not a list"
        }
    }
)
def update_cluster_variable_add_element_to_set(
    cluster_id: int,
    variable_id: int,
    request: models.UpdateClusterVariableRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterVariableResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)
    utils.validate_request(
        type(variable.value) is list,
        f"Can't add element to {variable.name}, because current value is not a list: {variable.value!r}."
    )

    if request.value not in variable.value:
        variable.value = variable.value + [request.value]
        db.commit()

        cluster = utils.get_cluster_or_404(db, cluster_id)
        background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.UpdateClusterVariableResponse(variable=variable)


@router.delete(
    "/{variable_id}/set",
    response_model=models.UpdateClusterVariableResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster or variable not found"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Can't delete element from set, because current value is not a list"
        }
    }
)
def update_cluster_variable_delete_element_from_set(
    cluster_id: int,
    variable_id: int,
    request: models.UpdateClusterVariableRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterVariableResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)
    utils.validate_request(
        type(variable.value) is list,
        f"Can't delete element from {variable.name}, because current value is not a list: {variable.value!r}."
    )

    if request.value in variable.value:
        # SQLAlchemy does not notice a change if we modify one of the elements in the list, or append/remove a value.
        # To make this work we should either make sure we assign a new list each time.
        saved_value = variable.value[:]
        saved_value.remove(request.value)
        variable.value = saved_value
        db.commit()

        cluster = utils.get_cluster_or_404(db, cluster_id)
        background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.UpdateClusterVariableResponse(variable=variable)


@router.put(
    "/{variable_id}/map",
    response_model=models.UpdateClusterVariableResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster or variable not found"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Can't add element to map, because current value is not a map"
        }
    }
)
def update_cluster_variable_add_element_to_map(
    cluster_id: int,
    variable_id: int,
    request: models.UpdateClusterVariableAsMapRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterVariableResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)
    utils.validate_request(
        type(variable.value) is dict,
        f"Can't add element to {variable.name}, because current value is not a map: {variable.value!r}."
    )

    # SQLAlchemy does not notice a change if we modify one of the elements in the dict, or add/remove a key-value pair.
    # To make this work we should either make sure we assign a new dict each time.
    saved_value = copy.copy(variable.value)
    saved_value[request.key] = request.value
    variable.value = saved_value
    db.commit()

    cluster = utils.get_cluster_or_404(db, cluster_id)
    background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.UpdateClusterVariableResponse(variable=variable)


@router.delete(
    "/{variable_id}/map",
    response_model=models.UpdateClusterVariableResponse,
    responses={
        status.HTTP_404_NOT_FOUND: {
            "model": models.ErrorResponse,
            "description": "Cluster or variable not found"
        },
        status.HTTP_412_PRECONDITION_FAILED: {
            "model": models.ErrorResponse,
            "description": "Can't delete element from map, because current value is not a map, or "
                           "specified key doesn't exist, or value has been changed by someone else"
        }
    }
)
def update_cluster_variable_add_element_to_map(
    cluster_id: int,
    variable_id: int,
    request: models.UpdateClusterVariableAsMapRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(db),
    uploader: AgentConfigS3Uploader = Depends(uploader),
    _: str = Depends(api_key)
) -> models.UpdateClusterVariableResponse:
    variable = utils.get_cluster_variable_or_404(db, cluster_id, variable_id)
    utils.validate_request(
        type(variable.value) is dict,
        f"Can't delete element from {variable.name}, because current value is not a map: {variable.value!r}."
    )

    utils.validate_request(
        request.key in variable.value,
        f"Key {request.key} doesn't exist in variable {variable.name}. Available keys: {list(variable.value.keys())}"
    )

    if request.key in variable.value:
        element = variable.value[request.key]
        utils.validate_request(
            element == request.value,
            "Can't update value, because variable has been changed by someone else: "
            f"{variable.name}[{request.key}] is {element!r}, not {request.value!r}"
        )

        # SQLAlchemy does not notice a change if we modify one of the elements in the dict, or add/remove a key.
        # To make this work we should either make sure we assign a new dict each time.
        saved_value = copy.copy(variable.value)
        del saved_value[request.key]
        variable.value = saved_value
        db.commit()

        cluster = utils.get_cluster_or_404(db, cluster_id)
        background_tasks.add_task(update_cluster_in_s3, db, uploader, cluster)

    return models.UpdateClusterVariableResponse(variable=variable)
