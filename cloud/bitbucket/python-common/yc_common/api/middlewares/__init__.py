class BaseMiddleware:
    """Basic methods for all middleware types"""

    @staticmethod
    def convert_json_request(request):
        return request
