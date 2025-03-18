# encoding=utf8
import library.python.charts_notes as notes
from antiadblock.configs_api.lib.db import UserLogins
import datetime


COMMENTS_CHANNELS = ['/AntiAdblock/partners_money2', '/AntiAdblock/partners_money_united']
SCALES = [('/minutely', 'i', 1), ('/hourly', 'h', 60), ('/daily', 'd', 1440)]
DOMAIN_GRAPH_FEED = 'antiadblock/domain_graph'

CONFIG_CHANGES_TEMPLATE = """https://antiblock.yandex.ru/service/{SERVICE_ID}/configs/diff/{OLD}/{NEW}"""
EVENT_TEMPLATE = """<br><a href="{LINK}" target="_blank" style="color: #4da2f1;">#{CONFIG_ID}</a> @{USER_NAME}"""
STRF_TMPL = {
    'i': "%Y-%m-%dT%H:%M:00.000Z",
    'h': "%Y-%m-%dT%H:00:00.000Z",
    'd': "%Y-%m-%d"
}


def get_current_time():
    now = datetime.datetime.utcnow() + datetime.timedelta(hours=3)
    return now - datetime.timedelta(minutes=now.minute % 10, seconds=now.second, microseconds=now.microsecond)


def find_user_name(user_id, blackbox):
    user_login = UserLogins.query.filter_by(uid=user_id).first()
    if user_login:
        return user_login.internallogin
    return blackbox.get_user_logins([user_id])[user_id]


class ChartsClient(object):

    def __init__(self, oauth_token, is_prod):
        self.oauth_token = oauth_token
        self.is_prod = is_prod

    def push_comment(self, user_id, service_id, new_config_id, old_config_id, blackbox):
        if not self.is_prod:
            return
        now = get_current_time()
        user_name = find_user_name(user_id, blackbox) or "Unknown"
        for channel in COMMENTS_CHANNELS:
            for scale in SCALES:
                feed = channel + scale[0]
                strf_str = STRF_TMPL[scale[1]]
                params = dict(service_id=["^{}^".format(service_id)], scale=scale[1])
                text = self._find_text_from_comment(
                    feed=feed,
                    date_from=now.strftime(strf_str),
                    date_to=(now + datetime.timedelta(minutes=scale[2])).strftime(strf_str),
                    params=params
                )
                text = text + EVENT_TEMPLATE.format(
                    LINK=CONFIG_CHANGES_TEMPLATE.format(SERVICE_ID=service_id, OLD=old_config_id, NEW=new_config_id),
                    CONFIG_ID=new_config_id,
                    USER_NAME=user_name
                )
                notes.create(feed=feed,
                             date=now.strftime(strf_str),
                             note=notes.Dot(text=text,
                                            graph_id=u'service_id=^{}^|unblock'.format(service_id),
                                            color=notes.Color.BLUE),
                             params=params,
                             oauth_token=self.oauth_token)
                notes.create(feed=DOMAIN_GRAPH_FEED,
                             date=now.strftime(strf_str),
                             note=notes.Line('<br>', color=notes.Color.YELLOW),
                             params=params,
                             oauth_token=self.oauth_token)

    def _find_text_from_comment(self, feed, date_from, date_to, params):
        for note in notes.fetch(feed=feed, date_from=date_from, date_to=date_to, oauth_token=self.oauth_token, raw=True):
            if note['params']['service_id'] == params['service_id']:
                notes.delete(note["id"], oauth_token=self.oauth_token)
                return note["text"]
        return ""
