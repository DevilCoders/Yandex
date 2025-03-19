# coding: utf-8

import datetime
import hmac
from hashlib import sha256
from typing import Iterable, List, Tuple, Union

import requests.models
import six
from six.moves import collections_abc
from six.moves.urllib.parse import quote, urlencode, urlsplit

from yc_requests.credentials import YandexCloudCredentials

SIGN_ALGORITHM = "YC_SET1_HMAC_SHA256"
SIGN_HEADER_NAMES = {"x-yacloud-signedheaders", "x-yacloud-signkeyservice", "x-yacloud-signkeydate",
                     "x-yacloud-signmethod", "x-yacloud-signature"}
EMPTY_SHA256_HASH = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"


class CanonicalRequestErrorCodes:
    HostHeaderNotSigned = 'HostHeaderNotSigned'


class CanonicalRequestError(Exception):
    def __init__(self, code, message):
        super(CanonicalRequestError, self).__init__(message)
        self.code = code
        self.message = message


class CanonicalRequest(object):
    def __init__(self,
                 method,                # type: str
                 path,                  # type: Union[str, None]
                 query_string,          # type: List[Tuple[str, str]]
                 headers,               # type: List[Tuple[str, List[str]]]
                 signed_header_names,   # type: List[str]
                 payload_checksum,      # type: str
                 request_time,          # type: str
                 ):
        self.method = self.normalize_method(method)
        self.path = self.normalize_path(path)
        self.query_string = self.normalize_query_string(query_string)
        self.signed_headers = self.normalize_signed_header_names(signed_header_names)
        self.headers = self.normalize_headers(headers, signed_header_names)
        self.payload_checksum = payload_checksum
        self.request_time = request_time

    def __str__(self):
        """Canonical request string"""
        template = "{method}\n"         \
                   "{path}\n"           \
                   "{query_string}\n"   \
                   "{headers}\n"        \
                   "{signed_headers}\n" \
                   "{payload_checksum}"
        return template.format(method=self.method, path=self.path, query_string=self.query_string, headers=self.headers,
                               signed_headers=self.signed_headers, payload_checksum=self.payload_checksum)

    def get_string_to_sign(self):
        # type: (...) -> str
        """String that represents normalized request and signing info. Signature is calculated from this string."""
        string_to_sign_template = "{sign_algorithm}\n"  \
                                  "{request_time}\n"    \
                                  "{request_len}\n"     \
                                  "{request_hash}."
        normalized_request_string = str(self)
        request_len = len(normalized_request_string)
        hashed_request = sha256(normalized_request_string.encode("utf-8")).hexdigest()
        return string_to_sign_template.format(sign_algorithm=SIGN_ALGORITHM, request_time=self.request_time,
                                              request_len=request_len, request_hash=hashed_request)

    @staticmethod
    def normalize_method(method):
        # type: (str) -> str
        """
        :param method: request method string
        
        Returns request method in uppercase.
        """
        return method.upper()

    @classmethod
    def normalize_path(cls, path):
        # type: (Union[str, None]) -> str
        """
        :param path: absolute path component of the URI
        
        Returns normalized (according to RFC 3986) and URI-encoded version of the absolute path component.
        """
        if not path:
            return "/"
        normalized_path = quote(cls.remove_dot_segments(path), safe="/~")
        return normalized_path

    @staticmethod
    def normalize_query_string(query_string):
        # type: (Iterable[Tuple[str, Union[str, Iterable[str]]]]) -> str
        """
        :param query_string: Iterable of query string parameters (key, value) or (key, [values]) tuples

        Returns normalized query string that meets the following requirements:

        1. Parameter names are sorted by character code point in ascending order. For example, a parameter name that
        begins with the uppercase letter F precedes a parameter name that begins with a lowercase letter b.

        2. Each parameter name and value are URI-encoded.

        3. Parameter name and value are separated with equals sign character (=).

        4. Parameters are separated with ampersand character (&). 
        """
        sorted_key_vals = []
        for key, values in sorted(query_string):
            if isinstance(values, (six.text_type, six.binary_type)) or not isinstance(values, collections_abc.Iterable):
                values = [values]

            if not isinstance(key, (six.text_type, six.binary_type)):
                key = str(key)
            for value in sorted(values):
                if value is not None:
                    if not isinstance(value, (six.text_type, six.binary_type)):
                        value = str(value)
                    # NOTE: py27 urllib does not quote values so we have to do it explicitly
                    sorted_key_vals.append("{}={}".format(quote(key, safe="-_.~"), quote(value, safe="-_.~")))

        normalized_query_string = "&".join(sorted_key_vals)
        return normalized_query_string

    def normalize_headers(self, headers, headers_to_sign):
        # type: (List[Tuple[str, List[str]]], List[str]) -> str
        """
        :param headers: List of header tuples. First element is key, second – list of values
        :param headers_to_sign: List of header names to sign

        Returns normalized headers string that meets the following requirements:

        1. String contains all request headers.

        2. Header names are lowercase with leading and trailing spaces removed.

        3. Header name and value are separated with colon character (:).

        4. Multiple header values are included unsorted and separated with comma character (,).

        5. Headers are sorted by character code and separated with newline character (\n).
        """
        headers_to_sign = [x.lower().strip() for x in headers_to_sign]
        normalized_headers = [(key.lower().strip(), ",".join(values)) for key, values in headers if key.lower().strip()
                              in headers_to_sign]
        return "\n".join("{}:{}".format(key, value) for (key, value) in sorted(normalized_headers))

    @staticmethod
    def normalize_signed_header_names(request_signed_header_names, blacklisted_headers=None):
        # type: (List[str], Union[List[str], None]) -> str
        """
        :param request_signed_header_names: List of request header names that are part of signing process
        :param blacklisted_headers: List of headers that are not part of signing process (optional)
        
        Returns normalized string that contains header names that are part of the signing process:

        1. Header names are lowercase with leading and trailing spaces removed.
        
        2. Header names are separated with semicolon character (;).

        3. The 'host' header must be included.
        """
        if blacklisted_headers is None:
            blacklisted_headers = []
        blacklisted_names_lowercase = {x.lower().strip() for x in blacklisted_headers}
        header_names_lowercase = ({x.lower().strip() for x in request_signed_header_names} -
                                  SIGN_HEADER_NAMES - blacklisted_names_lowercase)
        if "host" not in header_names_lowercase:
            raise CanonicalRequestError(CanonicalRequestErrorCodes.HostHeaderNotSigned,
                                        "'Host' header must be signed.")
        return ";".join(sorted(header_names_lowercase))

    @staticmethod
    def remove_dot_segments(url):
        # RFC 3986, section 5.2.4 "Remove Dot Segments"
        if not url:
            return ""

        input_url = url.split("/")
        output_list = []
        for x in input_url:
            if x and x != ".":
                if x == "..":
                    if output_list:
                        output_list.pop()
                else:
                    output_list.append(x)

        first = "/" if url[0] == "/" else ""
        last = "/" if url[-1] == "/" and output_list else ""
        return first + "/".join(output_list) + last


class AWSCanonicalRequest(CanonicalRequest):
    def __init__(self, *args, **kwargs):
        self.credential_scope = kwargs.pop("credential_scope")
        self.algorithm = kwargs.pop("algorithm")
        super(AWSCanonicalRequest, self).__init__(*args, **kwargs)

    def get_string_to_sign(self):
        string_to_sign_template = ("{sign_algorithm}\n"
                                   "{request_time}\n"
                                   "{credential_scope}\n"
                                   "{request_hash}")
        normalized_request_string = str(self)
        hashed_request = sha256(normalized_request_string.encode("utf-8")).hexdigest()
        return string_to_sign_template.format(sign_algorithm=self.algorithm, request_time=self.request_time,
                                              credential_scope=self.credential_scope, request_hash=hashed_request)

    @staticmethod
    def get_chunk_string_to_sign(request_time, credential_scope, prev_signature, chunk_hash):
        return ("AWS4-HMAC-SHA256-PAYLOAD\n"
                "{request_time}\n"
                "{credential_scope}\n"
                "{prev_signature}\n"
                "{empty_sha256}\n"
                "{chunk_hash}").format(
            request_time=request_time,
            credential_scope=credential_scope,
            prev_signature=prev_signature,
            empty_sha256=EMPTY_SHA256_HASH,
            chunk_hash=chunk_hash,
        )


class YcRequest(requests.models.RequestEncodingMixin, requests.models.Request):
    TIMESTAMP_FMT = "%Y%m%dT%H%M%SZ"

    def __init__(self, *args, **kwargs):
        requests.models.Request.__init__(self, *args, **kwargs)
        datetime_now = datetime.datetime.utcnow()
        self.timestamp = datetime_now.strftime(self.TIMESTAMP_FMT)
        # Host header is required for signing
        if 'host' not in self.headers:
            self.headers['host'] = urlsplit(self.url).netloc

    @property
    def body(self):
        p = requests.models.PreparedRequest()
        p.prepare_headers({})
        p.prepare_body(self.data, self.files, self.json)
        if isinstance(p.body, str):
            p.body = p.body.encode("utf-8")
        return p.body

    def make_canonical_request(self, blacklisted_headers=None):
        # type: (...) -> CanonicalRequest
        blacklisted_headers = set(blacklisted_headers or [])
        path = urlsplit(self.url).path
        if self.params:
            qs = self.params.items()
        else:
            # Construct query string from url
            qs = []
            parts = urlsplit(self.url)
            if parts.query:
                for pair in parts.query.split('&'):
                    key, _, value = pair.partition('=')
                    qs.append((key, value))
        headers = [(key, [value]) for key, value in self.headers.items()]
        signed_header_names = set([key.lower().strip() for key in self.headers.keys()]) - blacklisted_headers - SIGN_HEADER_NAMES
        empty_sha256_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        payload_checksum = sha256(self.body).hexdigest() if self.body else empty_sha256_hash
        return CanonicalRequest(method=self.method, path=path, query_string=qs, headers=headers,
                                signed_header_names=signed_header_names, payload_checksum=payload_checksum,
                                request_time=self.timestamp)


class RequestSigner:
    SIGNED_HEADERS_BLACKLIST = [
        "expect",
        "user-agent",
    ]

    def __init__(self, service_name, credentials):
        # type: (str, YandexCloudCredentials) -> self
        self._service_name = service_name
        self._credentials = credentials

    def sign(self, request):
        # type: (YcRequest) -> None

        # Set necessary headers before signing
        request.headers["X-YaCloud-SubjectToken"] = self._credentials.token
        request.headers["X-YaCloud-RequestTime"] = request.timestamp

        canonical_request = request.make_canonical_request(self.SIGNED_HEADERS_BLACKLIST)
        req_datestamp = canonical_request.request_time[0:8]
        signing_key = self.generate_signing_key(req_datestamp)
        string_to_sign = canonical_request.get_string_to_sign()
        signature = self._sign_msg_with_key(signing_key, string_to_sign, hex_digest=True)
        headers = {
            "X-YaCloud-SignedHeaders": canonical_request.signed_headers,
            "X-YaCloud-SignKeyService": self._service_name,
            "X-YaCloud-SignKeyDate": req_datestamp,
            "X-YaCloud-SignMethod": SIGN_ALGORITHM,
            "X-YaCloud-Signature": signature,
        }
        request.headers.update(headers)

    def generate_signing_key(self, datestamp):
        key = self._credentials.secret_key
        k_date = self._sign_msg_with_key(key, "YACLOUD" + datestamp)
        k_service = self._sign_msg_with_key(k_date, "Service" + self._service_name)
        k_signing = self._sign_msg_with_key(k_service, "YaCloud signed request")
        return k_signing

    @staticmethod
    def _sign_msg_with_key(key, msg, hex_digest=False):
        if not isinstance(key, (bytes, bytearray)):
            key = key.encode("utf-8")
        if not isinstance(msg, (bytes, bytearray)):
            msg = msg.encode("utf-8")
        sig = hmac.new(key, msg, sha256)
        return sig.hexdigest() if hex_digest else sig.digest()
