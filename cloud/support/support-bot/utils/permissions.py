#!/usr/bin/env python3
"""This module contains permission decorate funcs."""

import time
import logging

from decorator import decorate
from functools import wraps

from core.objects.user import User

logger = logging.getLogger(__name__)


def only_cloud_staff(func):
    """Checkout user permissions for access."""
    @wraps(func)
    def wrapper(upd, ctx, user=None, *args, **kwargs):
        if user is None:
            user = User.de_json(upd.message.chat.to_dict())

        if user.is_allowed:
            return func(upd, ctx, user=user, *args, **kwargs)
        else:
            logger.warning(f'Permission denied. User {user.telegram_login} trying to use command {func.__qualname__}')
    return wrapper


def only_existing_users(func):
    """Checkout the user existence (database)."""
    @wraps(func)
    def wrapper(upd, ctx, user=None, *args, **kwargs):
        if user is None:
            user = User.de_json(upd.message.chat.to_dict())

        if user._is_exist:
            return func(upd, ctx, user=user, *args, **kwargs)
        else:
            logger.warning(f'User {user.telegram_login} not exist, but trying to use command {func.__qualname__}')
    return wrapper


def admin_required(func):
    """Checkout the user admin permission."""
    @wraps(func)
    def wrapper(upd, ctx, user=None, *args, **kwargs):
        if user is None:
            user = User.de_json(upd.message.chat.to_dict())

        if user.config.is_admin:
            return func(upd, ctx, user=user, *args, **kwargs)
        else:
            logger.warning(f'User {user.telegram_login} not exist, but trying to use command {func.__qualname__}')
    return wrapper
