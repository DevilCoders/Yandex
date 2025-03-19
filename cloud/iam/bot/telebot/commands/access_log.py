import re

from pathlib import Path
from clickhouse_driver import Client

from cloud.iam.bot.telebot.commands import Command

INT_TO_GRPC_STATUS_CODE = {
    0: 'OK',
    1: 'CANCELLED',
    2: 'UNKNOWN',
    3: 'INVALID_ARGUMENT',
    4: 'DEADLINE_EXCEEDED',
    5: 'NOT_FOUND',
    6: 'ALREADY_EXISTS',
    7: 'PERMISSION_DENIED',
    8: 'RESOURCE_EXHAUSTED',
    9: 'FAILED_PRECONDITION',
    10: 'ABORTED',
    11: 'OUT_OF_RANGE',
    12: 'UNIMPLEMENTED',
    13: 'INTERNAL',
    14: 'UNAVAILABLE',
    15: 'DATA_LOSS',
    16: 'UNAUTHENTICATED'
}


class AccessLogItem:
    def __init__(self, authority, app, request_uri, duration, grpc_method, grpc_status_code,
                 remote_ip, user_agent, remote_calls_count, remote_calls_duration, request,
                 response, rest, timestamp):
        self.authority = authority
        self.app = app
        self.request_uri = request_uri
        self.duration = duration
        self.grpc_method = grpc_method
        self.grpc_status = INT_TO_GRPC_STATUS_CODE.get(grpc_status_code, str(grpc_status_code))
        self.remote_ip = remote_ip
        self.user_agent = user_agent
        self.remote_calls_count = remote_calls_count
        self.remote_calls_duration = remote_calls_duration
        self.request = request
        self.response = response
        self.rest = rest
        self.timestamp = timestamp


#
# /access-log <request_id>
#
class AccessLogCommand(Command):
    def __init__(self, bot, config_dir, config, jinja, paste_client):
        super().__init__(bot)

        password = Path(config_dir, config['password_file']).read_text()

        self._client = Client(host=config['host'],
                              port=config['port'],
                              database=config['database'],
                              user=config['user'],
                              password=password,
                              secure=config['secure'])

        self._help_message = """
 • `/access-log <request_id>` — search access logs
"""

        self._template = jinja.get_template('templates/access_log.html')
        self._paste_template = jinja.get_template('templates/access_log_details.paste')

        self._paste_client = paste_client

    def process(self, message):
        result = re.fullmatch(r'/access-log\s+([0-9A-Za-z_-]+)', message.text)
        if not result:
            return self._bot.send_message(chat_id=message.chat.id,
                                          reply_to_message_id=message.id,
                                          parse_mode='MarkdownV2',
                                          text='Invalid `/access-log` command format:\n' + self._help_message)

        request_id = result.group(1)

        results = self._client.execute("""
                                       SELECT authority,
                                              app,
                                              request_uri,
                                              duration,
                                              grpc_method,
                                              grpc_status_code,
                                              remote_ip,
                                              user_agent,
                                              remote_calls_count,
                                              remote_calls_duration,
                                              request,
                                              response,
                                              _rest,
                                              _timestamp
                                       FROM access_log
                                       WHERE request_id = %(request_id)s
                                         AND app = 'access-service'
                                       ORDER BY _timestamp
                                       """,
                                       {'request_id': request_id})

        access_log_items = list(map(lambda row: AccessLogItem(row[0], row[1], row[2], row[3], row[4], row[5], row[6],
                                                              row[7], row[8], row[9], row[10], row[11], row[12], row[13]),
                                    results))

        if len(access_log_items) > 0:
            paste_url = self._paste_client.post('plain', self._paste_template.render({'access_log_items': access_log_items}))
        else:
            paste_url = None

        self._bot.send_message(chat_id=message.chat.id,
                               reply_to_message_id=message.id,
                               parse_mode='HTML',
                               text=self._template.render({'access_log_items': access_log_items[:10],
                                                           'more_items': max(0, len(access_log_items) - 10),
                                                           'paste_url': paste_url}))
