import os
import re
from startrek_client import exceptions as st_exceptions
from startrek_client import Startrek
from time import sleep
from pathlib import Path
from shutil import rmtree

def drop_none(d, none=None):
    return {k: v for k, v in d.items() if v is not none}

def get_token():
    """
    Get startrek token from .st_token file
    :return: token string
    """
    with open('.st_token', 'r') as f:
        token = f.read()
    return token.strip()


class SupportST(Startrek):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    @staticmethod
    def cp_attach(issue_key, attachments=[]):

        Path(f"/tmp/{issue_key}").mkdir(parents=True, exist_ok=True)
        [attachment.download_to(f"/tmp/{issue_key}") for attachment in attachments]

    @staticmethod
    def up_attach(issue_key):
        return [f'/tmp/{issue_key}/{i}' for i in os.listdir(f"/tmp/{issue_key}")]

    @staticmethod
    def wipe_attach(issue_key):
        rmtree(f'/tmp/{issue_key}')

    def post_and_change_status(self, n_issue, text, add_tags=[], attachments=[], transition="partiallyCompleted"):

        issue = self.issues[n_issue.key]      # тут магия чтобы тикет пересхватился

        if 'english' in issue.components:
            reply_text = f'Hello! \n{parse_reply_tags(text)}'
        else:
            reply_text = f'Добрый день! \n{parse_reply_tags(text)}'
        self.cp_attach(issue.key, attachments)
        attaches = self.up_attach(issue.key)

        if parse_reply_tags(text) != '':
            data = drop_none({"text": reply_text,
                              "type": "outgoing",
                              "attachments": [i for i in attaches],
                              "params": {"expand": "attachments"}
                              })

            issue.comments.create(**data)
            try:
                self.wipe_attach(issue.key)
            except FileNotFoundError:
                print("no attachments found")

            print("{} commented".format(issue.key))
            try:
               if issue.transitions.get(transition):
                    issue.transitions[transition].execute()
                    sleep(1)
                    tags = issue.tags or []
                    issue = self.issues[issue.key]
                    issue.update(tags=tags + add_tags)
            except st_exceptions.NotFound:
                print("{} can not move to `{}`".format(issue.key, transition))
            else:
                print("{} moved to `{}`".format(issue.key, transition))


def has_outgoing(comments, type="outgoing"):
    """
    Check if there are comments of particular type among the tickets
    :param comments: <Comments> object
    :param type: string, default="outgoing"
    :return: bool
    """
    for c in reversed(list(comments.get_all())):
        if c.type == type:
            return True
    return False


def last_human_comment(comments):

    lastcomment = list(comments.get_all())[-1]
    if lastcomment.type == 'incoming':
        return True
    if lastcomment.type == 'standard':
        if 'Дополнительная информация' in lastcomment.text:
            return True
    return False


def parse_reply_tags(text: str):
    reply = '\n\r'.join(re.findall('\$\$\$for-customer\$\$\$(.*?)\$\$\$end-for-customer\$\$\$', text, re.DOTALL))
    return reply
