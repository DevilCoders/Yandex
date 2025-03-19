from startrek_client import exceptions as st_exceptions
from startrek_client import Startrek
from time import sleep


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

    def post_and_change_status(self, n_issue, text, add_tags=[], transition="partiallyCompleted"):
        n_issue = self.issues[n_issue.key]
        n_issue.comments.create(text=text,
                                type="outgoing",
                                )
        print("{} commented".format(n_issue.key))
        try:
           if n_issue.transitions.get(transition):
                n_issue.transitions[transition].execute()
                sleep(1)
                tags = n_issue.tags or []
                n_issue = self.issues[n_issue.key]
                n_issue.update(tags=tags + add_tags)
        except st_exceptions.NotFound:
            print("{} can not move to `{}`".format(n_issue.key, transition))
        else:
            print("{} moved to `{}`".format(n_issue.key, transition))


def has_outgoing(comments, type="outgoing"):
    """
    Check if there are comments of particular type among the tickets
    :param comments: <Comments> object
    :param type: string, default="outgoing"
    :return: bool
    """
    for c in comments.get_all():
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
