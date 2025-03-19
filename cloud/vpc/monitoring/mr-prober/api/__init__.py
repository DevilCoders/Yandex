import os
import traceback

from fastapi import FastAPI, Request
from fastapi.encoders import jsonable_encoder
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from fastapi_cache import FastAPICache
from fastapi_cache.backends.inmemory import InMemoryBackend
from opentelemetry import trace
from opentelemetry.exporter.jaeger.thrift import JaegerExporter
from opentelemetry.instrumentation.fastapi import FastAPIInstrumentor
from opentelemetry.instrumentation.sqlalchemy import SQLAlchemyInstrumentor
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import (
    ConsoleSpanExporter,
    SimpleSpanProcessor,
    BatchSpanProcessor
)
from prometheus_fastapi_instrumentator import Instrumentator
from starlette import status
from starlette.exceptions import HTTPException as StarletteHTTPException

import api.routes
import api.models
import database.models
import settings
import api.metrics


def setup_database():
    database.connect(settings.DATABASE_URL, settings.DATABASE_CONNECT_ARGS)


def setup_telemetry():
    trace.set_tracer_provider(TracerProvider())
    if settings.JAEGER_HOST != "":
        jaeger_exporter = JaegerExporter(
            agent_host_name=settings.JAEGER_HOST,
            agent_port=settings.JAEGER_PORT,
        )
        trace.get_tracer_provider().add_span_processor(
            BatchSpanProcessor(jaeger_exporter)
        )
    elif settings.DEBUG:
        trace.get_tracer_provider().add_span_processor(
            SimpleSpanProcessor(
                ConsoleSpanExporter(formatter=lambda span: span.to_json(indent=None) + os.linesep)
            )
        )


def setup_prometheus_instrumentator() -> Instrumentator:
    return Instrumentator(
        should_instrument_requests_inprogress=True,
        excluded_handlers=["/metrics", "/health", ],
        inprogress_name="inprogress",
        inprogress_labels=True,
    )


def setup_fastapi():
    app = FastAPI()

    FastAPIInstrumentor.instrument_app(app)
    SQLAlchemyInstrumentor().instrument(engine=database.engine, tracer_provider=trace.get_tracer_provider())

    prometheus_instrumentator = setup_prometheus_instrumentator()
    prometheus_instrumentator.instrument(app)
    prometheus_instrumentator.expose(app, include_in_schema=True, should_gzip=False)

    api.routes.register_all_routes(app)

    return app


setup_database()
setup_telemetry()
app = setup_fastapi()


@app.on_event("startup")
async def startup():
    # Start collecting metrics
    await api.metrics.get_agent_configuration_current_diff()
    FastAPICache.init(InMemoryBackend())


@app.on_event("shutdown")
async def shutdown():
    pass


@app.exception_handler(StarletteHTTPException)
async def http_exception_handler(request: Request, exception: StarletteHTTPException):
    return JSONResponse(
        content=jsonable_encoder(api.models.ErrorResponse(message=exception.detail)),
        status_code=exception.status_code
    )


@app.exception_handler(RequestValidationError)
async def validation_exception_handler(request: Request, exception: RequestValidationError):
    response = api.models.ErrorResponse(
        message="Invalid request",
        details={"errors": exception.errors(), "request": exception.body if exception.body else {}}
    )
    return JSONResponse(
        content=jsonable_encoder(response),
        status_code=status.HTTP_400_BAD_REQUEST
    )


@app.exception_handler(Exception)
async def exception_handler(request: Request, exception: Exception):
    response = api.models.ErrorResponse(
        message="Internal Server Error",
        details={
            "exception": str(exception),
            "traceback": list(traceback.TracebackException.from_exception(exception).format())
        }
    )
    return JSONResponse(
        content=jsonable_encoder(response),
        status_code=status.HTTP_500_INTERNAL_SERVER_ERROR
    )
