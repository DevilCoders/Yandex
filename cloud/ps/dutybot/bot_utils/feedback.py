from database.crud import create_query, read_query, update_query


class Feedback(object):
    __slots__ = ("id", "chat_id", "datetime", "component", "score", "comment")

    def __init__(
        self,
        id=None,
        chat_id=None,
        datetime=None,
        component=None,
        score=None,
        comment=None,
    ):
        self.id = id
        self.chat_id = chat_id
        self.component = component
        self.score = score
        self.datetime = datetime
        self.comment = comment

    def __dict__(self):
        return {_var: self.__getattribute__(_var) for _var in self.__slots__}

    def __delete_feedback_from_database(self):
        raise NotImplementedError()

    def add_feedback(self):
        fields = {"chat_id": self.chat_id, "component": self.component, "score": self.score}
        return create_query(table="feedback", data=fields)

    @classmethod
    def update_feedback(cls, id, comment):
        return update_query(table="feedback", condition=f"WHERE id={id}", fields={"comment": comment})

    @classmethod
    def get_last_feedback(cls, chat_id):
        try:
            _feedback = read_query(
                table="feedback",
                condition=f"WHERE chat_id = '{chat_id}' AND comment IS NULL AND "
                + "datetime > (current_timestamp - interval '1 day')"
                + "ORDER BY datetime DESC LIMIT 1",
            )[0]
            if _feedback:
                return Feedback(
                    id=_feedback[0],
                    chat_id=_feedback[1],
                    datetime=_feedback[2],
                    component=_feedback[3],
                    score=_feedback[4],
                    comment=_feedback[5],
                )
        except IndexError:
            return None
