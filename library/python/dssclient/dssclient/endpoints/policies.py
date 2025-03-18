from .base import EndpointSigning, Endpoint, EndpointUsers
from ..resources.policy import SigningPolicy, UsersPolicy


class Policies(Endpoint):
    """Предоставляет информацию о разного рода действующих политиках."""

    def get_signing_policy(self) -> SigningPolicy:
        """Возвращает объект политики, распространяющейся на подписывание документов."""
        return SigningPolicy.spawn(EndpointSigning(self._connector)._call('policy'), endpoint=self)

    def get_users_policy(self) -> UsersPolicy:
        """Возвращает объект политики, распространяющейся на управление пользователями сервиса."""
        return UsersPolicy.spawn(EndpointUsers(self._connector)._call('policy'), endpoint=self)
