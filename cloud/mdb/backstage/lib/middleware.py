class FixEmptyHostMiddleware:
    """
    Used to fix bug with response code 400 when binding to [::] and empty Host header.
    Django tries to use SERVER_NAME as host in django.http.request:HttpRequest.get_host()
    but "::" does not fit django.http.request:host_validation_re regular expression.
    This fix works with Django=1.8.4. Please review it when upgrading django.
    """
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        response = self.get_response(request)

        if 'HTTP_HOST' not in request.META and request.META.get('SERVER_NAME', '').startswith(':'):
            request.META['SERVER_NAME'] = 'localhost'

        return response
