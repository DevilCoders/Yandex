from pydantic import (
    BaseSettings,
    PyObject,
    SecretStr,
)
from typing import (
    List,
    Dict,
    Union,
    Optional,
)
from .types import BlackboxClient, PyBackendObject


class AsgiYauthConfig(BaseSettings):

    backends: List[PyBackendObject] = [
        'tvm2',
    ]

    ip_headers: List[str] = [
        'X-FORWARDED-FOR',
        'X-REAL-IP',
        'REMOTE-ADDR',
    ]

    tvm2_secret: Optional[SecretStr] = None
    tvm2_client: Optional[str] = None
    tvm2_allowed_clients: List[int] = []
    tvm2_blackbox_client: BlackboxClient = 'prod_yateam'
    tvm2_user_header = 'X-YA-USER-TICKET'
    tvm2_service_header = 'X-YA-SERVICE-TICKET'

    test_user_class: PyObject = 'asgi_yauth.user.YandexUser'
    test_user_data: Dict[str, Union[str, int, dict]] = {
        'uid': '123',
        'fields': {
            'email': 'test_user@yandex.ru',
        },
        'auth_type': 'test',
    }
    anonymous_user_class: PyObject = 'asgi_yauth.user.AnonymousYandexUser'

    class Config:
        env_prefix = 'yauth_'


config = AsgiYauthConfig()
