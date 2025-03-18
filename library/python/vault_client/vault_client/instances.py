from .client import VaultClient


VAULT_PRODUCTION_API = 'https://vault-api.passport.yandex.net'
VAULT_TESTING_API = 'https://vault-api-test.passport.yandex.net'


def Production(check_status=True, *args, **kwargs):
    return VaultClient(
        host=VAULT_PRODUCTION_API,
        check_status=check_status,
        *args,
        **kwargs
    )


def Testing(check_status=True, *args, **kwargs):
    return VaultClient(
        host=VAULT_TESTING_API,
        check_status=check_status,
        *args,
        **kwargs
    )


def Custom(host, check_status=False, *args, **kwargs):
    return VaultClient(
        host=host,
        check_status=check_status,
        *args,
        **kwargs
    )
