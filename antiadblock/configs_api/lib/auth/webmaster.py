import requests
from flask import current_app, g
from antiadblock.configs_api.lib.utils import URLBuilder


class WebmasterAPI(object):
    def __init__(self, context, webmaster_url):
        self.context = context
        self.webmaster = URLBuilder(webmaster_url)

    def get_user_domains(self):
        if g.get("webmaster_domains") is not None:
            return g.webmaster_domains
        user_ticket = self.context.auth.get_user_ticket()
        if not user_ticket:
            current_app.logger.error("Couldn't get domains from webmaster: can't get user_ticket")
            return []

        service_ticket = self.context.auth.get_webmaster_service_ticket()
        if service_ticket is None:
            raise Exception("Unable to get service ticket for webmaster")
        response = self._retrieve_domains_response(user_ticket, service_ticket)

        # hahn.[//home/webmaster/prod/export/archive/webmaster-verified-hosts/webmaster-verified-hosts.20180326] ->
        # all host_ids match (http|https):.*:\d{2,5}
        g.webmaster_domains = map(lambda verifiedHost: verifiedHost["hostId"].split(":")[1], response["verifiedHosts"])
        return g.webmaster_domains

    def _retrieve_domains_response(self, user_ticket, service_ticket):
        try:
            response = requests.get(str(self.webmaster["user"]["host"]["listUserHosts.json"]),
                                    verify=False,
                                    headers={"X-Ya-User-Ticket": user_ticket,
                                             "X-Ya-Service-Ticket": service_ticket})

            if response.status_code != 200 or "errors" in response.json():
                current_app.logger.error(
                    "Couldn't get domains from webmaster: {} response code".format(response.status_code),
                    extra=dict(response_code=response.status_code, response=response.content, user_ticket=user_ticket))
                g.webmaster_domains = []
                return dict(verifiedHosts=[])
            current_app.logger.debug("Got response from webmaster", extra=dict(response=response.content))
            return response.json()['data']
        except Exception as e:
            current_app.logger.error("Couldn't get domains from webmaster: {}".format(str(e))),
            g.webmaster_domains = []
            return dict(verifiedHosts=[])
