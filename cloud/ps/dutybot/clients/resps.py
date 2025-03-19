import dataclasses
import datetime
import functools
import logging
import operator
from typing import List

import urllib3
import urllib3.exceptions

from bot_utils.config import Config
from bot_utils.request import Requester


@dataclasses.dataclass
class ScheduleItem:
    team_name: str
    role: str
    start: datetime.datetime
    end: datetime.datetime


class RespsClient:
    def __init__(self):
        self.endpoint = Config.resps_endpoint
        self.verify = False
        self.order = {0: "primary", 1: "backup"}
        urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

    def get_service(self, name):
        url = f"{self.endpoint}/services/{name}"
        req = Requester().request(url=url, verify=self.verify)
        if req.get("error"):
            logging.error(req)
        return req

    def get_all_team_names(self):
        url = f"{self.endpoint}/services?fields=service"
        req = Requester().request(url=url, verify=self.verify)
        if not isinstance(req, list):
            return []
        return [team.get("service") for team in req]

    @functools.lru_cache(maxsize=50)
    def _get_service_name_by_id(self, id: str) -> str:
        url = f"{self.endpoint}/services/{id}"
        req = Requester().request(url=url, verify=self.verify)
        logging.info(f"Got service {req.get('service')} by id {id}")
        return req.get("service")

    def get_actual_oncall_by_team(self, team, timestamp: datetime.datetime):
        output = {}

        # GORE-99. If current time is 12:00, request timetable for 12:01, otherwise previous duties are returned too
        if timestamp.minute == 0:
            timestamp = timestamp.replace(minute=1)
        request_time = f"{timestamp.strftime('%d-%m-%YT%H:%m')}"

        url = f"{self.endpoint}/duty/{team}?from={request_time}&to={request_time}"
        req = Requester().request(url=url, verify=self.verify)

        if not isinstance(req, list):
            return output

        for engineer in req:
            oncall_type = self.order.get(engineer.get("resp", {}).get("order"))
            output[oncall_type] = engineer.get("resp", {}).get("username")

        return output

    def get_upcoming_duties_by_user(self, staff_login: str) -> List[ScheduleItem]:
        url = f"{self.endpoint}/duty/?user={staff_login}"
        response = Requester().request(url=url, verify=self.verify)
        if response is None:
            return []

        if not isinstance(response, list):
            raise ValueError(f"Unknown format of resps response: {url} returned {response}")

        now = datetime.datetime.now()
        schedule_items = []
        for service in response:
            start = datetime.datetime.fromtimestamp(service.get("datestart"))
            end = datetime.datetime.fromtimestamp(service.get("dateend"))

            # Skip already passed duties
            if end < now:
                continue

            service_name = self._get_service_name_by_id(service.get("serviceid"))
            shift_role = self.order.get(service.get("resp", {}).get("order"))
            schedule_items.append(ScheduleItem(service_name, shift_role, start, end))

        return sorted(schedule_items, key=operator.attrgetter("start"))


class RespsService:
    # TODO (andgein): object constructor SHOULD NOT make any HTTP requests except quite rare cases
    def __init__(self, name, timestamp: datetime.datetime):
        self.client = RespsClient()
        self.name = name
        self.raw_info = self.client.get_service(self.name)
        self.duty_ticket = self.raw_info.get("startrack", {}).get("duty", {}).get("last")
        self.teamlead = self.raw_info.get("teamowners", {}).get("lead")
        self.actual_oncall = self.client.get_actual_oncall_by_team(self.name, timestamp)
