from django.conf import settings
from django.shortcuts import HttpResponse, HttpResponseRedirect
from django.http.request import HttpRequest


def login_redirect(request: HttpRequest) -> HttpResponseRedirect:
    return HttpResponseRedirect(
        'https://passport.yandex-team.ru/auth?retpath=http://' + settings.BASE_HOST + request.GET.get('next')
    )


def ping(_: HttpRequest) -> HttpResponse:
    return HttpResponse('OK')
