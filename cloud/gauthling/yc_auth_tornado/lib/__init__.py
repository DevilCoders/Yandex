# Provide service names and canonical request to library users
from yc_requests import service_names
from yc_requests.signing import AWSCanonicalRequest, CanonicalRequest, CanonicalRequestError, CanonicalRequestErrorCodes

from yc_auth import exceptions
