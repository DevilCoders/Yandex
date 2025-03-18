import xml.etree.ElementTree as ET
from six.moves.urllib.parse import urljoin, urlparse

import requests
from requests import HTTPError
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

from django.conf import settings
from tvm2 import TVM2


class APIError(Exception):
    pass


class InvalidResponse(APIError):
    pass


class TVM2Error(APIError):
    pass


DEFAULT_CONNECTION_TIMEOUT = 5  # in sec.
DEFAULT_RETRY_COUNT = 5


class MDSClient(object):
    def __init__(self, read_host, write_host, namespace,
                 write_token, read_token=None, max_retries=None, timeout=None, use_tvm2=False,
                 allowed_hosts=None):
        """
        :param read_host: Read host
        :type read_host: str
        :param write_host: Write host (delete and update operations)
        :type write_host: str
        :param namespace: Namespace of the project
        :type namespace: str
        :param write_token: Base64 decoded auth pair login:passwd for write operations
        :type write_token: str
        :param read_token: Base64 decoded auth pair login:passwd for read operations
        :type read_token: str
        :param use_tvm2: authentication sign by tvm2
        :type use_tvm2: bool
        :param allowed_hosts: list allowed host names, comma separated string
        :type allowed_hosts: str

        """
        self._read_host = read_host
        self._write_host = write_host
        self._namespace = namespace
        self._timeout = timeout or DEFAULT_CONNECTION_TIMEOUT

        self._session = requests.Session()

        if max_retries is None:
            max_retries = Retry(
                DEFAULT_RETRY_COUNT,
                method_whitelist=('HEAD', 'GET', 'POST'),
                status_forcelist=(500, 502),
                backoff_factor=0.1
            )
        self._session.mount('http://', HTTPAdapter(max_retries=max_retries))
        self._session.mount('https://', HTTPAdapter(max_retries=max_retries))

        self._write_token = write_token
        self._read_token = read_token or write_token
        self._use_tvm2 = use_tvm2
        if use_tvm2:
            self._mds_storage_tvm2_client_id = getattr(settings, 'MDS_STORAGE_TVM2_CLIENT_ID', None)
            self._mds_client_tvm2_client_id = getattr(settings, 'MDS_CLIENT_TVM2_CLIENT_ID', None)
            self._mds_client_tvm2_secret = getattr(settings, 'MDS_CLIENT_TVM2_SECRET', None)
            self._mds_client_tvm2_blackbox_client = getattr(settings, 'MDS_CLIENT_TVM2_BLACKBOX_CLIENT', None)
            self.validate_tvm2_params()

        self._allowed_hosts = set()
        read_host_name = urlparse(self._read_host).hostname
        if read_host_name:
            self._allowed_hosts.add(read_host_name)
        if allowed_hosts:
            self._allowed_hosts.update(allowed_hosts.split(','))

    def validate_tvm2_params(self):
        attrs_to_check = (
            '_mds_storage_tvm2_client_id',
            '_mds_client_tvm2_client_id',
            '_mds_client_tvm2_secret',
            '_mds_client_tvm2_blackbox_client',
        )
        for attr in attrs_to_check:
            if not getattr(self, attr):
                raise AttributeError('%s is not defined in project settings' % attr.strip("_").upper())

    def _get_tvm2_client(self):
        return TVM2(
            client_id=self._mds_client_tvm2_client_id,
            secret=self._mds_client_tvm2_secret,
            blackbox_client=self._mds_client_tvm2_blackbox_client,
            destinations=(self._mds_storage_tvm2_client_id,),
        )

    def _get_tvm2_ticket(self):
        client = self._get_tvm2_client()
        response = client.get_service_tickets(self._mds_storage_tvm2_client_id)
        ticket = response.get(self._mds_storage_tvm2_client_id)
        if not ticket:
            raise TVM2Error("Can't get tvm2 ticket for MDS")
        return ticket

    @property
    def _read_session(self):
        if self._use_tvm2:
            tvm2_ticket = self._get_tvm2_ticket()
            self._session.headers.update({'X-Ya-Service-Ticket': tvm2_ticket})
        else:
            self._session.headers.update({'Authorization': 'Basic %s' % self._read_token})

        return self._session

    @property
    def _write_session(self):
        if self._use_tvm2:
            tvm2_ticket = self._get_tvm2_ticket()
            self._session.headers.update({'X-Ya-Service-Ticket': tvm2_ticket})
        else:
            self._session.headers.update({'Authorization': 'Basic %s' % self._write_token})

        return self._session

    def upload(self, data, filename, offset=None, expire=None):
        """
        Create or update file in MDS namespace

        :param data:  Filelike data to write
        :type data: FileIO
        :param filename: Filename
        :type filename: str
        :param offset: Write offset
        :type offset: int

        :raises: APIError on not ok(200) response
        """
        url = '{host}/upload-{namespace}/{filename}'.format(
            host=self._write_host,
            namespace=self._namespace,
            filename=filename
        )
        params = {}
        if offset:
            params['offset'] = offset

        if expire:
            params['expire'] = expire

        try:
            resp = self._write_session.post(
                url,
                params=params,
                data=data,
                timeout=self._timeout,
            )
            self._check_error(resp)
        except APIError:
            # https://wiki.yandex-team.ru/mds/dev/protocol/#put-errors
            if resp.status_code != 403:
                raise
            if not resp.content:
                # TTL error
                raise
        except requests.RequestException as e:
            raise APIError("Error: %s\nURL: %s" % (e, url))

        return self._parse_upload_response(resp.content)

    @staticmethod
    def _parse_upload_response(content):
        """
        Parse upload response XML

        :param content: XML content
        :type content: str

        :returns: File path in MDS namespace for read and delete operations
        :rtype: str
        """
        try:
            etree = ET.fromstring(content)
            key = etree.get('key')
            if key is None:
                # if file already exist, key located in another tag
                key = etree.find('key').text
            return key
        except Exception as e:
            raise InvalidResponse(e)

    def delete(self, path):
        """
        Delete file from MDS namespace

        :param path: Path to file in MDS namespace, \
        returned by upload method
        :type path: str

        :raises: APIError on not ok(200) response
        """
        url = '{host}/delete-{namespace}/{path}'.format(
            host=self._write_host,
            namespace=self._namespace,
            path=path
        )

        try:
            resp = self._write_session.get(url, timeout=self._timeout)
            self._check_error(resp)
        except requests.RequestException as e:
            raise APIError("Error: %s\nURL: %s" % (e, url))

    def get(self, path, offset=None, size=None):
        """
        Get file from MDS namespace

        :param path: Path to file in MDS namespace,
        returned by upload method
        :type path: str
        :param offset: Read offset
        :type offset: int
        :param size: How much to read
        :type size: int

        :raises: APIError on not ok(200) response

        :returns: file contents
        :rtype: str
        """
        if not path:
            return

        params = {}
        if offset:
            params['offset'] = offset
        if size:
            params['size'] = size

        url = self.read_url(path)
        try:
            resp = self._read_session.get(
                url,
                params=params,
                timeout=self._timeout
            )
            self._check_error(resp)
        except requests.RequestException as e:
            raise APIError("Error: %s\nURL: %s" % (e, url))

        return resp.content

    def _head(self, path):
        url = self.read_url(path)
        try:
            return self._read_session.head(url)
        except requests.RequestException as e:
            raise APIError("Error: %s\nURL: %s" % (e, url))

    def size(self, path):
        resp = self._head(path)
        self._check_error(resp)

        return resp.headers.get('content-length')

    def exists(self, path):
        """
        Check if file exists

        :param path: Path to file in MDS namespace, \
        returned by upload method
        :type path: str

        :raises: APIError if request is failed

        :rtype: bool
        """
        resp = self._head(path)
        if resp.status_code == 200:
            return True
        elif resp.status_code == 404:
            return False

        self._check_error(resp)

    def read_url(self, path):
        """
        Create GET url for path (group/filename) in current namespace

        :param path: Path to file in MDS namespace,
        returned by upload method
        :type path: str
        :returns: url
        :rtype: str
        """
        if not path:
            return

        if path.startswith('http'):
            # check here the domain in the given path parameter, because it is a possible security hole

            if not self._allowed_hosts:
                raise APIError('Neither read_host nor allowed_hosts has been defined')

            if urlparse(path).hostname in self._allowed_hosts:
                return path
            else:
                raise APIError('Unknown read host in given path: {}. Expected host with hostname some of: {}'.
                               format(path, ', '.join(self._allowed_hosts)))

        return urljoin(
            self._read_host,
            'get-{namespace}/{path}'.format(
                namespace=self._namespace,
                path=path
            )
        )

    @staticmethod
    def path_from_url(url):
        # http://mds_host/<action>-<namespace>/<couple_id>/<filename>
        if not url:
            return
        path_parts = urlparse(url).path.split('/')
        # ['', '<action>-<namespace>', '<couple_id>', '<filename>']
        return '/'.join(path_parts[2:])

    @staticmethod
    def _check_error(resp):
        """
        Check for error in response
        Useful if you need to suppress default HTTPError exceptions
        :param resp: :class: `requests.Response` instance
        :type resp: requests.Response
        """
        try:
            return resp.raise_for_status()
        except HTTPError as e:
            raise APIError("Error: %s\nURL: %s" % (e, resp.url))
