import django.http


def app(request):
    return django.http.HttpResponse('OK')
