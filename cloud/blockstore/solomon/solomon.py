import functools
import json
import logging
import time
import urllib2

SOLOMON_URL = "https://solomon.yandex-team.ru/api/v2/projects"
SOLOMON_TIMEOUT = 30


def retry(exception_to_check, tries=3, delay=3, backoff=2):
    def deco_retry(f):
        @functools.wraps(f)
        def f_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            while mtries > 1:
                try:
                    return f(*args, **kwargs)
                except exception_to_check, e:
                    logging.warning("%r, retrying in %r seconds...", e, mdelay)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return f(*args, **kwargs)
        return f_retry
    return deco_retry


def _hide_auth(headers):
    hidden = dict()
    for key, value in headers.iteritems():
        if value.strip().lower().startswith("oauth"):
            hidden[key] = "OAuth **HIDDEN**"
        else:
            hidden[key] = value
    return hidden


class RequestWithMethod(urllib2.Request):

    def __init__(self, *args, **kwargs):
        self._method = kwargs.pop("method", None)
        urllib2.Request.__init__(self, *args, **kwargs)

    def get_method(self):
        if self._method is not None:
            return self._method
        return urllib2.Request.get_method(self)


class SolomonError(Exception):
    pass


class RetriableSolomonError(SolomonError):
    pass


class SolomonObject(object):

    def __init__(self, project_id, object_id):
        self.project_id = project_id
        self.object_id = object_id

    def __hash__(self):
        return hash("{project_id}:{graph}".format(project_id=self.project_id, graph=self.object_id))

    def __eq__(self, other):
        return isinstance(other, self.__class__) \
            and self.object_id == other.object_id \
            and self.project_id == other.project_id


class Solomon(object):

    def __init__(self, auth, api_url=SOLOMON_URL, timeout=SOLOMON_TIMEOUT, dry_run=False):
        self.auth = auth
        self.api_url = api_url
        self.timeout = timeout
        self.dry_run = dry_run

    def _headers(self):
        headers = dict()
        application_json = "application/json"
        headers["Accept"] = application_json
        headers["Content-Type"] = application_json
        headers["Authorization"] = "OAuth " + self.auth
        return headers

    @retry(RetriableSolomonError)
    def _call(self, url, method=None, data=None):
        request_url = self.api_url + url
        logging.debug("API Call method %r url %s data %r headers %r",
                      method, request_url, data, _hide_auth(self._headers()))

        if self.dry_run and (data is not None or (method is not None and method.upper() != "GET")):
            return  # dry-run

        request_data = None
        if data is not None:
            request_data = json.dumps(data)

        request = RequestWithMethod(
            request_url, request_data, self._headers(), method=method)
        try:
            response = urllib2.urlopen(request, timeout=self.timeout)
        except urllib2.HTTPError as response:
            body = response.read()
            if response.code == 404:
                logging.warning("API Error 404: %r", body)
                return None
            if response.code > 500:
                logging.error("API request %s response error %d",
                              url, response.code)
                logging.error("API response body: %r", body)
                raise RetriableSolomonError("Solomon API error " + str(response.code))
            try:
                response_data = json.loads(body)
            except ValueError:
                logging.error(
                    "Invalid api response, can't decode json %r", body)
                raise SolomonError("Invalid json")
            try:
                logging.error("API Error code %r message %r",
                              response_data["code"], response_data["message"])
            except KeyError:
                logging.error("API Error %r", response_data)
            raise SolomonError("API HTTP Error " + str(response.code) + ": " + body)

        body = response.read()
        if len(body) == 0:
            return None
        try:
            return json.loads(body)
        except ValueError:
            logging.error("Invalid api response, can't decode json %r", body)
            raise SolomonError("Invalid json")

    def get_menu(self, project_id):
        return self._call("/" + project_id + "/menu")

    def put_menu(self, project_id, data):
        return self._call("/" + project_id + "/menu", "PUT", data)

    def get_dashboards(self, project_id):
        data = self._call("/" + project_id + "/dashboards?pageSize=5000")
        return [SolomonObject(project_id, item["id"]) for item in data["result"]]

    def get_dashboard(self, project_id, dashboard_id):
        return self._call("/" + project_id + "/dashboards/" + dashboard_id)

    def put_dashboard(self, project_id, dashboard_id, data):
        return self._call("/" + project_id + "/dashboards/" + dashboard_id, "PUT", data)

    def post_dashboard(self, project_id, data):
        return self._call("/" + project_id + "/dashboards", "POST", data)

    def delete_dashboard(self, project_id, dashboard_id):
        return self._call("/" + project_id + "/dashboards/" + dashboard_id, "DELETE")

    def get_graphs(self, project_id):
        data = self._call("/" + project_id + "/graphs?pageSize=5000")
        return [SolomonObject(project_id, item["id"]) for item in data["result"]]

    def get_graph(self, project_id, graph_id):
        return self._call("/" + project_id + "/graphs/" + graph_id)

    def put_graph(self, project_id, graph_id, data):
        return self._call("/" + project_id + "/graphs/" + graph_id, "PUT", data)

    def post_graph(self, project_id, data):
        return self._call("/" + project_id + "/graphs", "POST", data)

    def delete_graph(self, project_id, graph_id):
        return self._call("/" + project_id + "/graphs/" + graph_id, "DELETE")

    def get_clusters(self, project_id):
        data = self._call("/" + project_id + "/clusters?pageSize=5000")
        return [SolomonObject(project_id, item["id"]) for item in data["result"]]

    def get_cluster(self, project_id, cluster_id):
        return self._call("/" + project_id + "/clusters/" + cluster_id)

    def put_cluster(self, project_id, cluster_id, data):
        return self._call("/" + project_id + "/clusters/" + cluster_id, "PUT", data)

    def post_cluster(self, project_id, data):
        return self._call("/" + project_id + "/clusters", "POST", data)

    def delete_cluster(self, project_id, cluster_id):
        return self._call("/" + project_id + "/clusters/" + cluster_id, "DELETE")

    def get_services(self, project_id):
        data = self._call("/" + project_id + "/services?pageSize=5000")
        return [SolomonObject(project_id, item["id"]) for item in data["result"]]

    def get_service(self, project_id, service_id):
        return self._call("/" + project_id + "/services/" + service_id)

    def put_service(self, project_id, service_id, data):
        return self._call("/" + project_id + "/services/" + service_id, "PUT", data)

    def post_service(self, project_id, data):
        return self._call("/" + project_id + "/services", "POST", data)

    def delete_service(self, project_id, service_id):
        return self._call("/" + project_id + "/services/" + service_id, "DELETE")

    def get_shards(self, project_id):
        data = self._call("/" + project_id + "/shards?pageSize=5000")
        return [SolomonObject(project_id, item["id"]) for item in data["result"]]

    def get_shard(self, project_id, shard_id):
        return self._call("/" + project_id + "/shards/" + shard_id)

    def put_shard(self, project_id, shard_id, data):
        return self._call("/" + project_id + "/shards/" + shard_id, "PUT", data)

    def post_shard(self, project_id, data):
        return self._call("/" + project_id + "/shards", "POST", data)

    def delete_shard(self, project_id, shard_id):
        return self._call("/" + project_id + "/shards/" + shard_id, "DELETE")

    def get_project(self, project_id):
        return self._call("/" + project_id)

    def put_project(self, project_id, data):
        return self._call("/" + project_id, "PUT", data)

    def get_alerts(self, project_id):
        data = self._call("/" + project_id + "/alerts?pageSize=5000")
        return [SolomonObject(project_id, item["id"]) for item in data["items"]]

    def get_alert(self, project_id, alert_id):
        return self._call("/" + project_id + "/alerts/" + alert_id)

    def put_alert(self, project_id, alert_id, data):
        return self._call("/" + project_id + "/alerts/" + alert_id, "PUT", data)

    def post_alert(self, project_id, alert_id, data):
        return self._call("/" + project_id + "/alerts", "POST", data)

    def delete_alert(self, project_id, alert_id):
        return self._call("/" + project_id + "/alerts/" + alert_id, "DELETE")

    def get_channels(self, project_id):
        data = self._call("/" + project_id + "/notificationChannels?pageSize=5000")
        return [SolomonObject(project_id, item["id"]) for item in data["items"]]

    def get_channel(self, project_id, channel_id):
        return self._call("/" + project_id + "/notificationChannels/" + channel_id)

    def put_channel(self, project_id, channel_id, data):
        return self._call("/" + project_id + "/notificationChannels/" + channel_id, "PUT", data)

    def post_channel(self, project_id, channel_id, data):
        return self._call("/" + project_id + "/notificationChannels", "POST", data)

    def delete_channel(self, project_id, channel_id):
        return self._call("/" + project_id + "/notificationChannels/" + channel_id, "DELETE")
