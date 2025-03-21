__all__ = [
    'UserBonus',
    'UserBonusCreateRequestParameters',
]
import datetime
import decimal
import toloka.client.primitives.base
import toloka.client.primitives.parameter
import typing


class UserBonus(toloka.client.primitives.base.BaseTolokaObject):
    """Issuing a bonus to a specific performer

    It's addition to payment for completed tasks.

    Attributes:
        user_id: Performer ID to whom the bonus will be issued.
        amount: The bonus amount in dollars. Can be from 0.01 to 100 dollars per user per time.
        private_comment: Comments that are only visible to the requester.
        public_title: Message header for the user. You can provide a title in several languages
            (the message will come in the user's language). Format {'language': 'title', ... }.
            The language can be RU/EN/TR/ID/FR.
        public_message: Message text for the user. You can provide text in several languages
            (the message will come in the user's language). Format {'language': 'message', ... }.
            The language can be RU/EN/TR/ID/FR.
        without_message: Do not send a bonus message to the user. To award a bonus without a message, specify null
            for public_title and public_message and True for without_message.
        assignment_id: The answer to the task for which this bonus was issued.
        id: Internal ID of the issued bonus. Read only.
        created: Date the bonus was awarded, in UTC. Read only.

    Example:
        How to create bonus with message for specific assignment.

        >>> new_bonus = toloka_client.create_user_bonus(
        >>>     UserBonus(
        >>>         user_id='1',
        >>>         amount='0.50',
        >>>         public_title={
        >>>             'EN': 'Perfect job!',
        >>>         },
        >>>         public_message={
        >>>             'EN': 'You are the best performer EVER',
        >>>         },
        >>>         assignment_id='012345'
        >>>     )
        >>> )

        How to create bonus with message in several languages.

        >>> new_bonus = toloka_client.create_user_bonus(
        >>>     UserBonus(
        >>>         user_id='1',
        >>>         amount='0.10',
        >>>         public_title={
        >>>             'EN': 'Good Job!',
        >>>             'RU': 'Молодец!',
        >>>         },
        >>>         public_message={
        >>>             'EN': 'Ten tasks completed',
        >>>             'RU': 'Выполнено 10 заданий',
        >>>         }
        >>>     )
        >>> )
        ...
    """

    def __init__(
        self,
        *,
        user_id: typing.Optional[str] = None,
        amount: typing.Optional[decimal.Decimal] = None,
        private_comment: typing.Optional[str] = None,
        public_title: typing.Optional[typing.Dict[str, str]] = None,
        public_message: typing.Optional[typing.Dict[str, str]] = None,
        without_message: typing.Optional[bool] = None,
        assignment_id: typing.Optional[str] = None,
        id: typing.Optional[str] = None,
        created: typing.Optional[datetime.datetime] = None
    ) -> None:
        """Method generated by attrs for class UserBonus.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    user_id: typing.Optional[str]
    amount: typing.Optional[decimal.Decimal]
    private_comment: typing.Optional[str]
    public_title: typing.Optional[typing.Dict[str, str]]
    public_message: typing.Optional[typing.Dict[str, str]]
    without_message: typing.Optional[bool]
    assignment_id: typing.Optional[str]
    id: typing.Optional[str]
    created: typing.Optional[datetime.datetime]


class UserBonusCreateRequestParameters(toloka.client.primitives.parameter.Parameters):
    """Parameters for creating performer bonuses

    Used in methods 'create_user_bonus', 'create_user_bonuses' и 'create_user_bonuses_async' of the class TolokaClient,
    to clarify the behavior when creating bonuses.

    Attributes:
        operation_id: Operation ID. If asynchronous creation is used, by this identifier you can later get
            results of creating bonuses.
        skip_invalid_items: Validation parameters of objects:
            * True - Award a bonus if the object with bonus information passed validation. Otherwise, skip the bonus.
            * False - Default behaviour. Stop the operation and don't award bonuses if at least one object didn't pass validation.
    """

    def __init__(
        self,
        *,
        operation_id: typing.Optional[str] = None,
        skip_invalid_items: typing.Optional[bool] = None
    ) -> None:
        """Method generated by attrs for class UserBonusCreateRequestParameters.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    operation_id: typing.Optional[str]
    skip_invalid_items: typing.Optional[bool]
