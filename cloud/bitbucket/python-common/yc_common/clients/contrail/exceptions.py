from yc_common.exceptions import Error


class HttpError(Error):
    """The base exception class for all HTTP exceptions."""

    http_status = 0
    message = "HTTP Error"

    def __init__(self, message=None, details=None,
                 response=None, request_id=None,
                 url=None, method=None, http_status=None,
                 retry_after=0):
        self.http_status = http_status or self.http_status
        self.message = message or self.message
        self.details = details
        self.request_id = request_id
        self.response = response
        self.url = url
        self.method = method
        formatted_string = "{} (HTTP {})".format(self.message, self.http_status)
        self.retry_after = retry_after
        if request_id:
            formatted_string += " (Request-ID: {})".format(request_id)
        super().__init__(formatted_string)


class AbsPathRequiredError(Error):
    pass


class ResourceMissingError(Error):
    pass


class NotFoundError(Error):

    def __init__(self, message="No resource found", resource=None, collection=None):
        self.r = resource
        self.c = collection
        super().__init__(message)


class ResourceNotFoundError(NotFoundError):

    def __init__(self, *args, **kwargs):
        r = kwargs.get("resource")
        if r is None:
            message = "Resource not found"
        else:
            message = "Resource {} not found".format(r.path) \
                if r.path.is_uuid \
                else "Resource {}/{} not found".format(r.type, str(r.fq_name))
        kwargs["message"] = message
        super().__init__(*args, **kwargs)


class CollectionNotFoundError(NotFoundError):

    def __init__(self, *args, **kwargs):
        c = kwargs.get("collection")
        if c is None:
            message = "Collection not found"
        else:
            message = "Collection {} not found".format(c.path)
        kwargs["message"] = message
        super().__init__(*args, **kwargs)


class ExistsError(Error):

    def __init__(self, resources=None):
        self.resources = []
        if resources is not None:
            self.resources = resources
        message = "{}({})".format(self.__class__.__name__,
                                  ", ".join([repr(r) for r in self.resources]))
        super().__init__(message)

    @property
    def _paths(self):
        return ", ".join([str(c.path) for c in self.resources])


class ChildrenExistsError(ExistsError):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.message = "Children {} exists".format(self._paths)


class BackRefsExistsError(ExistsError):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.message = "Back references from {} exists".format(self._paths)

