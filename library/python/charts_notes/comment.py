from .notes import note_from_json
from .utils import parse_date


class Comment(object):
    """
    Comment placed on feed
    """
    def __init__(
        self,
        comment_id,
        creator_login, created_date,
        modifier_login, modified_date,
        feed,
        note,
        date, date_until=None,
        params=None,
    ):
        self.id = comment_id
        self.creator_login = creator_login,
        self.created_date = parse_date(created_date)
        self.modifier_login = modifier_login
        self.modified_date = parse_date(modified_date)
        self.feed = feed
        self.note = note
        self.date = parse_date(date)
        self.date_until = parse_date(date_until) if date_until is not None else None
        self.params = params or {}

    def __repr__(self):
        return '<{comment_id}: {note_type} on {feed} at {date} with text: {text}>'.format(
            comment_id=self.id,
            note_type=self.note.NOTE_TYPE,
            feed=self.feed,
            date=self.date if self.date_until is None else (self.date, self.date_until),
            text=self.note.text,
        )

    @staticmethod
    def from_json(data):
        return Comment(
            comment_id=data['id'],
            creator_login=data['creatorLogin'],
            created_date=data['createdDate'],
            modifier_login=data['modifierLogin'],
            modified_date=data['modifiedDate'],
            feed=data['feed'],
            note=note_from_json(data),
            date=data['date'], date_until=data.get('dateUntil'),
            params=data.get('params'),
        )
