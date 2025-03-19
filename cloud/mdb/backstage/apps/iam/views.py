import django.http as http

import cloud.mdb.backstage.apps.iam.lib as lib
import cloud.mdb.backstage.apps.iam.session_service

import cloud.mdb.backstage.lib.helpers as mod_helpers


session_service = cloud.mdb.backstage.apps.iam.session_service.SessionService()


def callback(request):
    response = http.HttpResponseRedirect('/')

    code = request.GET.get('code')
    access_token = lib.get_access_token(code)
    session = session_service.create(access_token)

    for cookie in session.set_cookie_header:
        spl = cookie.split('=')
        name = spl[0]
        value = '='.join(spl[1:])
        response.set_cookie(name, value)

    return response


def debug(request):
    ctx = {
        'menu': '',
    }
    html = mod_helpers.render_html('iam/views/debug.html', ctx, request)
    return http.HttpResponse(html)
