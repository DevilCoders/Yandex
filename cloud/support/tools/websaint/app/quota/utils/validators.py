#!/usr/bin/env python3
"""This module contains validate functions."""

import re
import sys
import logging

from app.quota.utils.helpers import Color
from app.quota.error import ServiceError, ValidateError
from app.quota.constants import SERVICE_ALIASES, SERVICES

logger = logging.getLogger(__name__)


def validate_subject_id(subject_id: str):
    if re.fullmatch(r'^b1+[a-zA-Z0-9]{18}$', subject_id) or re.fullmatch(r'^ao+[a-zA-Z0-9]{18}$', subject_id):
        return subject_id.strip()

    if subject_id in ('q', 'quit', 'abort', 'exit'):
        sys.exit(Color.make('\nAborted by "quit" command', color='yellow'))

    raise ValidateError(f'Invalid subject id received: {subject_id}')


def validate_service(service: str):
    valid_services = ', '.join(SERVICES)
    aliases = ', '.join(SERVICE_ALIASES)
    logger.debug(f'Supported services: {valid_services}')
    logger.debug(f'Service aliases: {aliases}')
    error = f'Unknown service "{service}" received.'

    if service is None:
        raise ServiceError(error)

    if service in ('q', 'quit', 'abort', 'exit'):
        sys.exit(Color.make('\nAborted by "quit" command', color='yellow'))

    service = SERVICE_ALIASES[service] if service in SERVICE_ALIASES else service
    if service not in SERVICES:
        raise ServiceError(error)

    return service


def validate_multiplier(obj: str):
    if len(obj) < 2:
        return False

    if obj.startswith('x'):
        try:
            obj = int(obj[1:])
            if obj < 2:
                raise ValidateError('The multiplier must be greater than one. Example: x2')
            return obj
        except (TypeError, ValueError):
            raise ValidateError('The multiplier must be greater than one. Example: x2')

    return False
