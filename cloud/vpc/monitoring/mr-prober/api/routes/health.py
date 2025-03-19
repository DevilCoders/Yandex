from fastapi import Depends, APIRouter
from sqlalchemy.orm import Session

from api import models
from api.dependencies import db

router = APIRouter(
    prefix="/health",
    tags=["health"],
)


@router.get("", response_model=models.SuccessResponse)
def check_health(_: Session = Depends(db)) -> models.SuccessResponse:
    return models.SuccessResponse()
