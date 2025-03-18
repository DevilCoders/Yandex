import logging

import yenv
import time
from enum import IntEnum
from typing import List
from pydantic import BaseSettings

from library.python.redis_utils import RedisSentinelSettings
from library.python.celery_dashboard.serializers import get_serializer
from library.python.celery_dashboard.core.task_context import TaskContext, TaskStatus


logger = logging.getLogger('celery_dashboard')


class Timing(IntEnum):
    SECOND = 1
    MINUTE = 60 * SECOND
    HOUR = 60 * MINUTE
    DAY = 24 * HOUR
    WEEK = 7 * DAY
    MONTH = 30 * DAY


class BaseSettingsConfig:
    env_file = f'celery_dashboard_{yenv.type}.env'
    env_file_encoding = 'utf-8'


class DashboardSettings(BaseSettings):
    is_active: bool = False
    serializer_name: str = 'msgpack'

    class Config(BaseSettingsConfig):
        env_prefix = 'celery_dashboard_'


class RedisSettings(RedisSentinelSettings):
    hosts: List[str] = ['localhost']
    db: int = 3

    class Config(BaseSettingsConfig):
        env_prefix = 'celery_dashboard_redis_'


class DashboardIOWrapper:
    @staticmethod
    def suppress(func):
        def decorator(cls: 'DashboardIO', *args, **kwargs):
            try:
                func(cls, *args, **kwargs)
            except Exception as e:
                logger.exception('Dashboard error:', exc_info=e)

        return decorator

    @staticmethod
    def check_is_active(func):
        def decorator(cls: 'DashboardIO', *args, **kwargs):
            if cls.dashboard_settings.is_active:
                func(cls, *args, **kwargs)

        return decorator

    @staticmethod
    def with_pipeline(func):
        def decorator(cls: 'DashboardIO', *args, **kwargs):
            cls.create_pipeline()
            func(cls, *args, **kwargs)
            cls.execute_and_close_pipeline()

        return decorator

    @staticmethod
    def pre_proceed(func):
        def decorator(cls: 'DashboardIO', *args, **kwargs):
            cls.pre_proceed(*args, **kwargs)
            func(cls, *args, **kwargs)

        return decorator


class DashboardIO:
    def __init__(self):
        self.redis_settings = RedisSettings()
        self.dashboard_settings = DashboardSettings()
        self.redis = self.redis_settings.get_redis_master()
        self.serializer = get_serializer(self.dashboard_settings.serializer_name)

        self.pipeline = None
        self._now = None
        self._key = None
        self._args_kwargs = None

    def create_pipeline(self):
        self.pipeline = self.redis.pipeline()

    def execute_and_close_pipeline(self):
        self.pipeline.execute()
        self.pipeline = None

    def pre_proceed(self, task_context: TaskContext):
        self._now = int(time.time())
        self._key = self.serializer.serialize([task_context.name, task_context.args_kwargs])
        self._args_kwargs = self.serializer.serialize(task_context.args_kwargs)

        self._register_task_name(task_context)

        self.pipeline.hmset(
            self._key,
            dict(
                task_name=task_context.name,
                last_status=task_context.status,
            )
        )

        self._update_task_info(task_context)

    @DashboardIOWrapper.suppress
    @DashboardIOWrapper.check_is_active
    @DashboardIOWrapper.with_pipeline
    @DashboardIOWrapper.pre_proceed
    def store_success_task(self, task_context: TaskContext):
        self._register_task_name_success(task_context)

    @DashboardIOWrapper.suppress
    @DashboardIOWrapper.check_is_active
    @DashboardIOWrapper.with_pipeline
    @DashboardIOWrapper.pre_proceed
    def store_failed_task(self, task_context: TaskContext):
        self._register_task_name_error(task_context)

    @DashboardIOWrapper.suppress
    @DashboardIOWrapper.check_is_active
    @DashboardIOWrapper.with_pipeline
    @DashboardIOWrapper.pre_proceed
    def store_retried_task(self, task_context: TaskContext):
        self._register_task_name_error(task_context)

    def _register_task_name(self, task_context: TaskContext):
        self.pipeline.sadd('task_names', task_context.name)

    def _register_task_name_success(self, task_context: TaskContext):
        self.pipeline.zrem(f'{task_context.name}_errors', self._args_kwargs)
        self.pipeline.delete(self._key)

    def _register_task_name_error(self, task_context: TaskContext):
        self.pipeline.zadd(f'{task_context.name}_errors', {self._args_kwargs: self._now})

        einfo = task_context.extra.get('einfo', None)
        exc = task_context.extra.get('exc', None)

        self.pipeline.hmset(
            self._key,
            dict(
                traceback=None if not einfo else einfo.traceback,
                exception=None if not exc else str(exc),
                task_id=task_context.id,
            )
        )

    def _update_task_info(self, task_context: TaskContext):
        status = task_context.status

        self.pipeline.hset(self._key, f'last_{status}_dt', self._now)
        self.pipeline.hincrby(self._key, f'{status}_count', 1)
        self.pipeline.hset(f'{task_context.name}_info', f'last_{status}_dt', self._now)

        ttl = (Timing.WEEK
               if status == TaskStatus.SUCCESS
               else Timing.MONTH).value

        self.pipeline.expire(self._key, ttl)
        self.pipeline.expire(f'{task_context.name}_info', ttl)
