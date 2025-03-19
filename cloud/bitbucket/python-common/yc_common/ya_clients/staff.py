from urllib.parse import urlencode

from yc_common import logging
from yc_common.metrics import Metric, MetricTypes, monitor
from yc_common.ya_clients.common import IntranetClient, chunks, LazyRequest, LazyResponse


log = logging.get_logger(__name__)

requests = Metric(
    MetricTypes.COUNTER, "staff_client_requests",
    ["type", "path"], "Staff client request counter.")


class StaffClient(IntranetClient):
    _client_name = "staff"

    def __init__(self, client_id, token, base_url, timeout):
        super().__init__(base_url=base_url, auth_type="oauth", client_id=client_id, token=token, timeout=timeout)

    @monitor(requests, labels={"path": "get_persons"})
    def get_users_by_login(self, user_logins):
        log.debug("Requesting users from Staff")
        urls = []
        for chunk in chunks(user_logins, 10):
            query = urlencode({"login": "," .join(chunk),
                               "_fields": ",".join(["id", "uid", "login", "work_email",
                                                    "official.is_dismissed", "name",
                                                    "department_group.department.name"]),
                               "_sort": "id"})
            urls.append("persons?{}".format(query))
        return LazyRequest(self, urls)

    @monitor(requests, labels={"path": "get_persons"})
    def get_users(self, user_uids):
        log.debug("Requesting users from Staff")
        urls = []
        for chunk in chunks(user_uids, 10):
            query = urlencode({"uid": "," .join(chunk),
                               "_fields": ",".join(["id", "uid", "login", "work_email",
                                                    "official.is_dismissed", "name",
                                                    "department_group.department.name"]),
                               "_sort": "id"})
            urls.append("persons?{}".format(query))
        return LazyRequest(self, urls)

    @monitor(requests, labels={"path": "get_persons"})
    def list_users(self):
        log.debug("Requesting users from Staff")
        query = urlencode({"_fields": ",".join(["id", "uid", "login", "work_email",
                                                "official.is_dismissed", "name",
                                                "department_group.department.name"]),
                           "_limit": 300,
                           "_sort": "id"})
        response = self.get("persons?{}".format(query))
        return LazyResponse(self, response)

    @monitor(requests, labels={"path": "get_groups"})
    def get_departments(self, departments_ids):
        log.debug("Requesting departments for specified ids from Staff")
        urls = []
        for chunk in chunks(departments_ids, 10):
            query = urlencode({"id": ",".join(map(str, chunk)),
                               "type": "department",
                               "_limit": 300,
                               "_fields": ",".join(["id", "name", "type", "url"]),
                               "_sort": "id"})
            urls.append("groups?{}".format(query))
        return LazyRequest(self, urls)

    @monitor(requests, labels={"path": "get_groups"})
    def list_departments(self):
        log.debug("Requesting groups from Staff")
        query = urlencode({"type": "department",
                           "_fields": ",".join(["id", "name", "type", "url"]),
                           "_limit": 300,
                           "_sort": "id"})
        response = self.get("groups?{}".format(query))
        return LazyResponse(self, response)

    @monitor(requests, labels={"path": "get_group_membership"})
    def get_group_members(self, search):
        query = {"_fields": ",".join(["person"]),
                 "_sort": "id"}
        query.update(search)
        response = self.get("groupmembership?{}".format(urlencode(query)))
        return LazyResponse(self, response)

    def list_group_members_by_id(self, group_id):
        log.debug("Requesting group (id: %s) memebers from Staff", group_id)
        return self.get_group_members({"group.id": group_id})

    def list_group_members_by_url(self, group_url):
        log.debug("Requesting group (url: %s) memebers from Staff", group_url)
        return self.get_group_members({"group.url": group_url})
