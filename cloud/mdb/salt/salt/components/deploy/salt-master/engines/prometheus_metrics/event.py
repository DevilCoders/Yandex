"""
Salt events connoisseur.
Attempt to categorize Salt-Master events.

Most of the code taken from
https://github.com/pengyao/salt-metrics/blob/master/saltmetrics/utils/event.py
"""

import re
import logging

import six


log = logging.getLogger(__name__)


class EventFinger:
    def __init__(self):
        self.tag = "other"
        self.func = ""
        self.success = None
        self.state = ""
        self.retcode = -1

    def __eq__(self, other):
        return self.__dict__ == other.__dict__


_JID = r"\d{20}"
_MINION_ID = r"[\.\w-]+"
# Some events and their meaning
# can be found in doc:
# https://docs.saltproject.io/en/latest/topics/event/master_events.html
_FINGERS = {
    "auth": r"salt/auth$",
    "wheel_new": r"salt/wheel/{}/new$".format(_JID),
    "wheel_ret": r"salt/wheel/{}/ret$".format(_JID),
    "run_new": r"salt/run/{}/new$".format(_JID),
    "run_ret": r"salt/run/{}/ret$".format(_JID),
    "job_new": r"salt/job/{}/new$".format(_JID),
    "job_ret": r"salt/job/{}/ret/{}$".format(_JID, _MINION_ID),
    "minion_start": r"^minion_start$",
    "minion_ping": r"^minion_ping$",
    "minion_refresh": r"^minion/refresh/",
    "key": r"^salt/key$",
    "ext_processes_test": r"^ext_processes/test",
}


class EventConnoisseur:
    """
    Event Connoisseur
    """

    def __init__(self):
        self._event_fingers = {}
        for key, value in six.iteritems(_FINGERS):
            self._event_fingers[key] = re.compile(value)

    def get_finger(self, event):
        """
        Get finger from event
        """
        finger = EventFinger()
        if not isinstance(event, dict):
            return finger
        event_tag = event.get("tag", "")
        event_data = event.get("data", dict())
        for tag, rule in six.iteritems(self._event_fingers):
            if rule.match(event_tag):
                finger.tag = tag
                if tag.endswith("_ret") and event_data:
                    # Count function on return,
                    # cause we can get it status (success, retcode)
                    finger.func = event_data.get("fun", "")
                    finger.success = event_data.get("success")
                    finger.retcode = event_data.get("retcode")
                    if finger.func == "state.sls":
                        # State name is a first parameter of state.sls
                        try:
                            finger.state = event_data["fun_args"][0]
                        except (KeyError, TypeError) as exc:
                            log.warning(
                                "Failed to extract state name from event %r: %s",
                                event_data,
                                exc,
                            )
                if tag in ("key", "auth"):
                    act = event_data.get("act")
                    if act:
                        finger.tag = "%s_%s" % (tag, act)
                break
        if finger.tag == "other":
            if event_tag.isdigit():
                # Ignore lots of:
                #
                #   {u'tag': '20210713161700143379', u'data': {u'_stamp': u'2021-07-13T16:17:00.143650', u'minions'
                #       :[u'wizard-internal-api-test-01i.db.yandex.net']}}
                #
                # Looks a like that them appears from find_job
                #
                #   Published command details {u'tgt_type': 'list', u'jid': u'20210713161700143379',
                #       u'tgt': ['wizard-internal-api-test-01i.db.yandex.net'],
                #       u'ret': '', u'user': 'mdb-deploy-salt-api',
                #       u'arg': ['20210713161655021541'], u'fun': 'saltutil.find_job'}
                return None
            log.debug("EventConnoisseur can't define event finger: %r", event)
        return finger
