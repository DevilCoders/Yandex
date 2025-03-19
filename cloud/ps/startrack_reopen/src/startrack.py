# -*- coding: UTF-8 -*-

import requests
import time
import logging


class StartrekClient:
    def __init__(self, token, config):
        self.token = token
        self.rps = 1.0 / config["rps_limit"]["get"]
        self.rps_post = 1.0 / config["rps_limit"]["post"]
        self.url_get_ticket = config["base_url"]
        self.filter_string = config["filter"]
        self.headers = {
            "Authorization": "OAuth {token}".format(token=token),
            "Content-Type": "application/json",
        }

    def get_tickets_by_filter(self):
        link = "{0}?{1}".format(self.url_get_ticket, self.filter_string)
        data = []
        while True:
            time.sleep(self.rps)  # limit rps
            try:
                r = requests.get(link, headers=self.headers)

                if r.status_code != 200:
                    continue

                next_page = r.links.get("next")
                if not next_page:
                    data.extend(r.json())
                    break

                link = next_page["url"]
            except Exception as e:
                logging.exception(
                    "Got an exception during %s: %s", "get_tickets_by_filter", e
                )
                return None
            else:
                data.extend(r.json())

        return data

    def get_ticket(self, ticket):
        try:
            r = requests.get(
                "{0}/{1}".format(self.url_get_ticket, ticket), headers=self.headers
            )
            data = r.json()
            if r.status_code != 200:
                return logging.error("Got status code %d while getting ticket %s, unable to proceed", r.status_code, ticket)
        except Exception as e:
            return logging.exception("Got an exception during %s: %s", "get_ticket", e)
            # return None
        else:
            return data

    def get_last_comments(self, ticket):
        link = "{0}/{1}/comments?perPage=1000".format(self.url_get_ticket, ticket)
        data = []
        while True:
            time.sleep(self.rps)  # limit rps
            try:
                r = requests.get(
                    link,
                    headers=self.headers,
                )

                if r.status_code != 200:
                    continue

                next_page = r.links.get("next")
                if not next_page:
                    data.extend(r.json())
                    break

                link = next_page["url"]
            except Exception as e:
                logging.exception(
                    "Got an exception during %s: %s", "get_last_comments", e
                )
                return None, None
            else:
                data.extend(r.json())

        std = [x for x in data if x["type"] == "standard"]
        if len(std) < 1:
            std.append({})

        out = [x for x in data if x["type"] == "outgoing"]
        if len(out) < 1:
            out.append({})

        return std[-1], out[-1]

    def get_transitions(self, ticket):
        try:
            r = requests.get(
                "{0}/{1}/transitions".format(self.url_get_ticket, ticket),
                headers=self.headers,
            )
            data = r.json()
        except Exception as e:
            logging.exception("Got an exception during %s: %s", "get_transitions", e)
            return None
        else:
            return data

    def get_links(self, ticket):
        try:
            r = requests.get(
                "{0}/{1}/links".format(self.url_get_ticket, ticket),
                headers=self.headers,
            )
            data = r.json()
        except Exception as e:
            logging.exception("Got an exception during %s: %s", "get_links", e)
            return None
        else:
            return data

    def new_comment(self, ticket, text, summonees):
        req = {
            "text": text,
            "summonees": summonees if summonees else [],
            "attachmentIds": [],
            "maillistSummonees": [],
        }
        logging.info("Summoning %s", req["summonees"])

        try:
            r = requests.post(
                "{0}/{1}/comments".format(self.url_get_ticket, ticket),
                headers=self.headers,
                json=req,
            )
            data = r.json()
        except Exception as e:
            logging.exception("Got an exception during %s: %s", "new_comment", e)
            return {"error": True, "errorMessages": e}
        else:
            return data

    def translate(self, ticket, status):
        possible_trans = self.get_transitions(ticket)
        if status not in [x["id"] for x in possible_trans]:
            logging.exception(
                "Not possible to translate ticket %s to: %s", ticket, status
            )
            return {"errorMessages": "Not possible to translate to: {0}".format(status)}

        try:
            r = requests.post(
                "{0}/{1}/transitions/{2}/_execute".format(
                    self.url_get_ticket, ticket, status
                ),
                headers=self.headers,
            )
            data = r.json()
        except Exception as e:
            logging.exception("Got an exception during %s: %s", "translate", e)
            return {"error": True, "errorMessages": e}
        else:
            return data

    def remove_deadline(self, ticket):
        req = {"deadline": None}
        logging.info("Removing deadline")

        try:
            r = requests.patch(
                "{0}/{1}/".format(self.url_get_ticket, ticket),
                headers=self.headers,
                json=req
            )
            data = r.json()
        except Exception as e:
            logging.exception("Got an exception during %s: %s", "remove_deadline", e)
            return {"error": True, "errorMessages": e}

        return data

    def update_tags(self, ticket, tags):
        req = {"tags": tags}
        logging.info("Inserting tag")
        try:
            r = requests.patch(
                "{0}/{1}/".format(self.url_get_ticket, ticket),
                headers=self.headers,
                json=req
            )
            data = r.json()
        except Exception as e:
            logging.exception("Got an exception during %s: %s", "insert_tag", e)
            return {"error": True, "errorMessages": e}

        return data
