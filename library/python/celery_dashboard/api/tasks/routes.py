from typing import List

from fastapi import APIRouter

from .models import TaskInfo, TaskError
from library.python.celery_dashboard.core import RedisSettings, DashboardSettings
from library.python.celery_dashboard.serializers import get_serializer


router = APIRouter()

redis_settings = RedisSettings()
dashboard_settings = DashboardSettings()

redis = redis_settings.get_redis_slave()

serializer = get_serializer(dashboard_settings.serializer_name)


@router.get("/", response_model=List[TaskInfo])
async def get_all_tasks(failed_only: bool = False):
    task_names = sorted(redis.smembers('task_names'))

    pipeline = redis.pipeline()
    for task_name in task_names:
        pipeline.hgetall(f'{task_name.decode()}_info')  # type: ignore
    tasks_info = pipeline.execute()

    tasks_summary = zip(task_names, tasks_info)

    return [
        TaskInfo(
            name=task_name,
            **decode_dict_keys(task_info)
        )
        for task_name, task_info in tasks_summary
        if not failed_only or task_info.get(b'last_failed_dt', None) or task_info.get(b'last_retry_dt', None)
    ]


@router.get("/{task_name}/", response_model=List[TaskError])
async def get_task(task_name: str):
    task_args_kwargs = redis.zrange(f'{task_name}_errors', 0, -1)
    task_args_kwargs = list(map(serializer.deserialize, task_args_kwargs))

    task_keys = [serializer.serialize([task_name, args_kwargs]) for args_kwargs in task_args_kwargs]

    pipeline = redis.pipeline()
    [pipeline.hgetall(key) for key in task_keys]
    pipeline_results = iter(pipeline.execute())

    return [
        TaskError(
            name=task_name,
            args=[f'{a}' for a in args],
            kwargs=kwargs,
            **decode_dict_keys(task_error),
        )
        for (args, kwargs), task_error in zip(task_args_kwargs, pipeline_results)
    ]


def decode_dict_keys(d: dict):
    return {k.decode(): v for k, v in d.items()}
