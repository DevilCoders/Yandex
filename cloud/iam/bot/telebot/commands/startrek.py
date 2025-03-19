import datetime
import re

from cloud.iam.bot.telebot.commands import Command


#
# /startrek worklog
#
class StartrekCommand(Command):
    def __init__(self, bot, jinja, startrek_client):
        super().__init__(bot)

        self._help_message = """
 • `/startrek worklog [date]` — show my worklog
 • `/startrek worklog delete <issue_key> <worklog_id>` — delete my worklog
"""

        self._template_worklog = jinja.get_template('templates/startrek_worklog.html')

        self._startrek_client = startrek_client

    def _invalid_command_format(self, message):
        return self._bot.send_message(chat_id=message.chat.id,
                                      reply_to_message_id=message.id,
                                      parse_mode='MarkdownV2',
                                      text='Invalid `/startrek` command format:\n' + self._help_message)

    def _worklog(self, message, date):
        # TODO Use staff timezone, respect date param.
        date = datetime.date.today()

        worklog_items = self._startrek_client.worklog(message.authenticated_user.login, date, date + datetime.timedelta(days=1))

        self._bot.send_message(chat_id=message.chat.id,
                               reply_to_message_id=message.id,
                               parse_mode='HTML',
                               text=self._template_worklog.render({'worklog_items': worklog_items,
                                                                   'date': date,
                                                                   'startrek_url': self._startrek_client.ui_url}))

    def _delete_worklog(self, message, issue_key, worklog_id):
        worklog = self._startrek_client.find_worklog(issue_key, worklog_id)
        if not worklog:
            self._bot.send_message(chat_id=message.chat.id,
                                   reply_to_message_id=message.id,
                                   parse_mode='MarkdownV2',
                                   text='Not found')
            return False

        if worklog['createdBy']['id'] != message.authenticated_user.login:
            self._bot.send_message(chat_id=message.chat.id,
                                   reply_to_message_id=message.id,
                                   parse_mode='MarkdownV2',
                                   text='You can delete only worklogs you created')
            return False

        if self._startrek_client.delete_worklog(issue_key, worklog_id):
            self._bot.send_message(chat_id=message.chat.id,
                                   reply_to_message_id=message.id,
                                   parse_mode='MarkdownV2',
                                   text='Done')
        else:
            self._bot.send_message(chat_id=message.chat.id,
                                   reply_to_message_id=message.id,
                                   parse_mode='MarkdownV2',
                                   text='Cannot delete the worklog')

    def process(self, message):
        result = re.fullmatch(r'/(startrek|st)\s+(worklog|wl)(\s+((delete|del)\s+([A-Za-z0-9-]+)\s+([0-9]+)|([0-9]{4}-[0-9]{2}-[0-9]{2})))?', message.text)
        if not result:
            return self._invalid_command_format(message)

        command = result.group(2)
        delete_issue_key = result.group(6)
        delete_worklog_id = result.group(7)
        date = result.group(8)

        if command == 'worklog' or command == 'wl':
            if delete_issue_key:
                return self._delete_worklog(message, delete_issue_key, delete_worklog_id)
            else:
                return self._worklog(message, date)
        else:
            return self._invalid_command_format(message)
