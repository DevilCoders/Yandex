# -*- coding: UTF-8 -*-

import time
import logging

from datetime import datetime, timezone, timedelta
from enum import Enum

from cloud.ps.startrack_reopen.config import cfg


TIME_FORMAT = "%Y-%m-%dT%H:%M:%S.%f%z"
MSK = timezone(timedelta(hours=3))


class CompanyOwnerType(Enum):
    Individual = 0
    Enterprise = 1


class Check:
    def __init__(self, stc, ticket):
        self.stc = stc
        if ticket:
            self.ticket_id = ticket["key"]
            std, out = stc.get_last_comments(self.ticket_id)

            self.pending_reply = [x["id"] for x in ticket.get("pendingReplyFrom", [])]
            self.status = ticket["status"]["key"]
            self.last_updated = ticket["updatedAt"]
            self.company_name = ticket.get("companyName")
            self.company_plan = ticket.get("pay")
            self.deadline = ticket.get("deadline")
            self.tags = ticket.get("tags", [])
            self.components = ticket.get("components", [])
            self.action = self.rule_select()["action"]

            self.last_comment_internal_date = std.get("createdAt")
            self.last_comment_external_date = out.get("createdAt")

            self.company_type = self.get_owner_type()
        else:
            raise Exception(
                "Ticket {0} could not be processed".format(self.ticket_id),
                self.ticket_id,
            )

    def get_priority(self):
        if self.company_plan in cfg["company_filters"]["priority"]["high"]:
            return True

        return False

    def get_owner_type(self):
        if (
            self.company_name
            and self.company_name
            not in cfg["company_filters"]["owner_type"]["individual"]
        ):
            return CompanyOwnerType.Enterprise

        return CompanyOwnerType.Individual

    def is_a_good_time(self, time, start, end, days):
        localtime = time.astimezone(MSK)
        if (
            localtime.hour >= start
            and localtime.hour <= end
            and localtime.weekday() in days
        ):
            return True

        return False

    def reopen_and_comment(self):
        logging.info(
            "Executing %s action at %s %s %s",
            self.status,
            self.ticket_id,
            self.company_type,
            self.company_plan,
        )
        ans = self.stc.translate(
            self.ticket_id, self.action[self.status]["opening_status"]
        )
        if isinstance(ans, dict):
            logging.info("Reopen task failed with error: %s", ans.get("errorMessages"))
            return

        ans = self.stc.new_comment(
            self.ticket_id,
            self.action[self.status]["text"],
            self.pending_reply if self.action[self.status]["summon"] else None,
        )
        if ans.get("errors"):
            logging.info("Comment task failed with error: %s", ans.get("errorMessages"))

        if "reopen" in self.tags:
            return

        self.tags.append("reopen")
        self.stc.update_tags(self.ticket_id, self.tags)

        time.sleep(self.stc.rps_post)

    def rule_select(self):
        for name, rule in cfg["rules"].items():
            # [tracker, compute, vpc, wiki] x [tracker, wiki, forms] = [tracker, wiki]
            if len(set(rule.get("condition", {}).get("component", [])).intersection(set([x["display"] for x in self.components]))) > 0:
                logging.info("Ticket %s corresponds to rule %s", self.ticket_id, name)
                return rule

        logging.info("Ticket %s corresponds to rule %s", self.ticket_id, "default")
        return cfg["rules"]["default"]

    def resolve(self):
        now = datetime.now(timezone.utc)
        if self.status == "waitForAnswer":
            if (
                self.company_type == CompanyOwnerType.Enterprise
                and not self.is_a_good_time(now, 12, 21, [0, 1, 2, 3, 4])
            ):  # until saturday
                return

            if self.deadline:
                if now > datetime.strptime(self.deadline, "%Y-%m-%d").astimezone(MSK):
                    self.reopen_and_comment()
                    self.stc.remove_deadline(self.ticket_id)
                return

            if self.get_priority():
                crit_delta = 48 * 3600
            else:
                crit_delta = 24 * 3600
            if self.last_comment_external_date:
                delta = now - datetime.strptime(
                    self.last_comment_external_date, TIME_FORMAT
                )
                if delta.total_seconds() > crit_delta:
                    self.reopen_and_comment()

        elif self.status == "needInfo":
            if not self.is_a_good_time(now, 12, 21, [0, 1, 2, 3, 4]):
                return

            if self.deadline:
                if now > datetime.strptime(self.deadline, "%Y-%m-%d").astimezone(MSK):
                    self.reopen_and_comment()
                    self.stc.remove_deadline(self.ticket_id)
                return

            if self.get_priority():
                crit_delta = 3 * 3600
            else:
                crit_delta = 24 * 3600

            delta = now - datetime.strptime(
                self.last_comment_internal_date, TIME_FORMAT
            )
            if delta.total_seconds() > crit_delta:
                self.reopen_and_comment()

        elif self.status == "awaitingDeployment":
            if not self.is_a_good_time(now, 12, 21, [0, 1, 2, 3, 4]):
                return

            if self.deadline:
                if now > datetime.strptime(self.deadline, "%Y-%m-%d").astimezone(MSK):
                    self.reopen_and_comment()
                    self.stc.remove_deadline(self.ticket_id)
                return

            depends = [x for x in self.stc.get_links(self.ticket_id) if x["type"]["id"] == "depends"]
            states = [x["status"]["key"] == "closed" for x in depends]
            if all(states) and len(states) > 0:
                self.reopen_and_comment()
                return

            crit_delta = 2 * 7 * 24 * 3600
            delta = now - datetime.strptime(self.last_updated, TIME_FORMAT)
            if delta.total_seconds() > crit_delta:
                self.reopen_and_comment()

        else:
            logging.info("Ticket {} is in wrong status: {}".format(self.ticket_id, self.status))
