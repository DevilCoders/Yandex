import uvicorn

import settings

if __name__ == "__main__":
    uvicorn.run(
        "api:app",
        host=settings.HOST,
        port=settings.PORT,
        debug=settings.DEBUG,
        reload=settings.API_AUTO_RELOAD,
        log_config=settings.LOGGING_CONFIG
    )
