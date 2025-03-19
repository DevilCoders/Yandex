import requests
import urllib3
import logging
import time
from datetime import datetime, timedelta

from app.utils.config import Config

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
resps_endpoint = Config.resps_endpoint
order = {0: "primary",
         1: "backup"}


class RespsException(Exception):
    """Happens while resps is down
    No handle, just abort function and call uplink."""

def service_id_to_name(id):
    with requests.Session() as r:
        try:
            req = r.get(f"{resps_endpoint}/services/{id}",
                        verify=False)
            team = req.json()
            if not req.ok:
                raise RespsException()
            return team.get("service")
        except RespsException:
            logging.info(f"[resps.get_duty_ticket] http_code {req.status_code}; data {team}")
            return None


def epoch_to_human(epoch):
    date_format = "%Y-%m-%d %H:%M:%S"
    date = time.strftime(date_format, time.localtime(epoch))
    return datetime.strptime(date, date_format)


def get_user_upcoming_duties(staff):
    with requests.Session() as r:
        try:
            date_format = "%Y-%m-%d %H:%M:%S"
            output = []
            req = r.get(f"{resps_endpoint}/duty/?user={staff}",
                        verify=False)
            schedule = req.json()
            if not req.ok:
                raise RespsException()
            for service in schedule:
                diff = (epoch_to_human(service.get('dateend')) - datetime.now()).total_seconds()
                if diff < 0:
                    continue
                name = service_id_to_name(service.get('serviceid'))
                start = datetime.strftime(epoch_to_human(service.get('datestart')), date_format)[:10]
                end = datetime.strftime(epoch_to_human(service.get('dateend')), date_format)[:10]
                role = order.get(service.get("resp").get("order"))
                output.append([name, start, end, role])
            return output
        except RespsException:
            logging.info(f"[resps.get_duty_ticket] http_code {req.status_code}; data {schedule}")
            return []


def get_duty_ticket(team):
    with requests.Session() as r:
        try:
            req = r.get(f"{resps_endpoint}/services/{team}",
                        verify=False)
            if not req.ok:
                    raise RespsException()
            team_json = req.json()
            return team_json.get('startrack').get('duty').get('last')
        except (KeyError, RespsException, AttributeError) as e:
            return None


def get_teams_list():
    with requests.Session() as r:
        try:
            # Get data from resps
            req = r.get(f"{resps_endpoint}/services")
            teams_json = [team.get('service') for team in req.json()]
            if not req.ok:
                    raise RespsException()
            return teams_json
        except (KeyError, RespsException, TypeError):
            logging.info(f"[resps.get_teams_list] http_code {req.status_code}; data {teams_json}")
            return []


def get_oncall(team, date, end=False):
    with requests.Session() as r:
            try:
                if end is False:
                    end = date
                output = {}
                req = r.get(f"{resps_endpoint}/duty/{team}?from={date}T13:00&to={end}T13:00",
                            verify=False)
                data = req.json()
                if not req.ok:
                    raise RespsException()
                if data is None:
                    return output
                for oncall in data:
                    output[order.get(oncall.get('resp').get('order'))] = oncall.get('resp').get('username')
                return output
            except RespsException:
                logging.info(f"[resps.get_oncall] http_code {req.status_code}; data {data}")
                return {}
