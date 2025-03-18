# coding: utf-8
from django import http
from django.utils.http import is_safe_url

from django_yauth import user, models
from django_yauth.util import get_setting


def create_profile(request):
    if not request.yauser.is_authenticated():
        return http.HttpResponseForbidden('Authentication required')

    if get_setting(['CREATE_PROFILE_ON_ACCESS', 'YAUTH_CREATE_PROFILE_ON_ACCESS']):
        for model in models.get_profile_models():
            user.create_profile(request.yauser, model)

    if get_setting(['CREATE_USER_ON_ACCESS', 'YAUTH_CREATE_USER_ON_ACCESS']):
        user.create_user(request.yauser)

    redirect_to = request.POST.get('next', request.GET.get('next', ''))
    if not is_safe_url(redirect_to, request.get_host()):
        return http.HttpResponseForbidden('Forbidden redirect location')

    return http.HttpResponseRedirect(redirect_to)
