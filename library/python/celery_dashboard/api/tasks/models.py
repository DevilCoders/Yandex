from typing import Optional

from pydantic import BaseModel


class TaskInfo(BaseModel):
    name: str
    last_success_dt: Optional[float]
    last_retry_dt: Optional[float]
    last_failed_dt: Optional[float]


class TaskError(TaskInfo):
    args: list
    kwargs: dict
    task_id: Optional[str]
    retry_count: Optional[int]
    failed_count: Optional[int]
    success_count: Optional[int]
    exception: Optional[str]
    traceback: Optional[str]
    last_status: Optional[str]
