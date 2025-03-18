import tvmauth
from django.conf import settings


def get_client():
    c = tvmauth.TvmClient(
        tvmauth.TvmToolClientSettings(
            self_alias=settings.ABC_DATA_TVM_CLIENT_ID,
        )
    )
    return c
