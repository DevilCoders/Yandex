import importlib

from .clients.base import BaseClient


def get_client(client: str, **kwargs) -> BaseClient:
    path = f'async_clients.clients.{client}'
    module = importlib.import_module(path)
    return module.Client(**kwargs)
