# -*- coding: utf-8 -*-
"""
Utility functions for dealing with feature flags.
"""

from typing import Sequence

from flask import g

from ..core.exceptions import FeatureUnavailableError


def get_feature_flags() -> Sequence[str]:
    """
    Returns feature flags available in the context of the currently
    executing request.
    """
    return getattr(g, 'cloud', {}).get('feature_flags', [])


def ensure_feature_flag(name: str) -> None:
    """
    Ensures that the given feature flag is enabled in the context of
    the currently executing request, or raises FeatureUnavailableError.
    """
    if name not in get_feature_flags():
        raise FeatureUnavailableError()


def ensure_no_feature_flag(name: str) -> None:
    """
    Ensures that the given feature flag is not enabled in the context of
    the currently executing request, or raises FeatureUnavailableError.
    """
    if name in get_feature_flags():
        raise FeatureUnavailableError()


def has_feature_flag(name: str) -> bool:
    """
    Returns true if `name` flag available in the context of the currently
    executing request.
    """
    return name in get_feature_flags()
