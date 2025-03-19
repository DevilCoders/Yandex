from fastapi import Security, HTTPException
from fastapi.security import APIKeyHeader
from starlette import status

import database
import settings


def db():
    session = database.session_maker()
    try:
        yield session
    finally:
        session.close()


def api_key(api_key_header: str = Security(APIKeyHeader(name="X-API-KEY", auto_error=False))):
    if api_key_header == settings.API_KEY:
        return api_key_header

    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid X-API-KEY header"
    )
