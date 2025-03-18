from fastapi import FastAPI
from fastapi.responses import RedirectResponse

from . import tasks


app = FastAPI()
api_prefix = '/dashboard-api'


@app.on_event('startup')
async def startup_event():
    # ToDo: redis_connect
    pass


@app.get('/')
def read_root():
    return RedirectResponse(f'{api_prefix}/tasks')


app.include_router(
    tasks.router,
    prefix=f'{api_prefix}/tasks',
    tags=['tasks'],
)
