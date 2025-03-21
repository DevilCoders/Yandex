__all__ = [
    'Assignment',
    'AssignmentPatch',
    'GetAssignmentsTsvParameters',
]
import datetime
import decimal
import toloka.client.primitives.base
import toloka.client.primitives.parameter
import toloka.client.solution
import toloka.client.task
import toloka.util._extendable_enum
import typing


class Assignment(toloka.client.primitives.base.BaseTolokaObject):
    """Contains information about an assigned task suite and the results

    Attributes:
        id: ID of the task suite assignment to a performer.
        task_suite_id: ID of a task suite.
        pool_id: ID of the pool that the task suite belongs to.
        user_id: ID of the performer who was assigned the task suite.
        status: Status of an assigned task suite.
            * ACTIVE - In the process of execution by the performer.
            * SUBMITTED - Completed but not checked.
            * ACCEPTED - Accepted by the requester.
            * REJECTED - Rejected by the requester.
            * SKIPPED - Skipped by the performer.
            * EXPIRED - The time for completing the tasks expired.
        reward: Payment received by the performer.
        tasks: Data for the tasks.
        automerged: Flag of the response received as a result of merging identical tasks. Value:
            * True - The response was recorded when identical tasks were merged.
            * False - Normal performer response.
        created: The date and time when the task suite was assigned to a performer.
        submitted: The date and time when the task suite was completed by a performer.
        accepted: The date and time when the responses for the task suite were accepted by the requester.
        rejected: The date and time when the responses for the task suite were rejected by the requester.
        skipped: The date and time when the task suite was skipped by the performer.
        expired: The date and time when the time for completing the task suite expired.
        first_declined_solution_attempt: For training tasks. The performer's first responses in the training task
            (only if these were the wrong answers). If the performer answered correctly on the first try, the
            first_declined_solution_attempt array is omitted.
            Arrays with the responses (output_values) are arranged in the same order as the task data in the tasks array.
        solutions: performer responses. Arranged in the same order as the data for tasks in the tasks array.
        mixed: Type of operation for creating a task suite:
            * True - Automatic ("smart mixing").
            * False - Manually.
        public_comment: Public comment about an assignment. Why it was accepted or rejected.
    """

    class Status(toloka.util._extendable_enum.ExtendableStrEnum):
        """An enumeration.
        """

        ACTIVE = 'ACTIVE'
        SUBMITTED = 'SUBMITTED'
        ACCEPTED = 'ACCEPTED'
        REJECTED = 'REJECTED'
        SKIPPED = 'SKIPPED'
        EXPIRED = 'EXPIRED'

    def __init__(
        self,
        *,
        id: typing.Optional[str] = None,
        task_suite_id: typing.Optional[str] = None,
        pool_id: typing.Optional[str] = None,
        user_id: typing.Optional[str] = None,
        status: typing.Union[Status, str, None] = None,
        reward: typing.Optional[decimal.Decimal] = None,
        tasks: typing.Optional[typing.List[toloka.client.task.Task]] = None,
        automerged: typing.Optional[bool] = None,
        created: typing.Optional[datetime.datetime] = None,
        submitted: typing.Optional[datetime.datetime] = None,
        accepted: typing.Optional[datetime.datetime] = None,
        rejected: typing.Optional[datetime.datetime] = None,
        skipped: typing.Optional[datetime.datetime] = None,
        expired: typing.Optional[datetime.datetime] = None,
        first_declined_solution_attempt: typing.Optional[typing.List[toloka.client.solution.Solution]] = None,
        solutions: typing.Optional[typing.List[toloka.client.solution.Solution]] = None,
        mixed: typing.Optional[bool] = None,
        public_comment: typing.Optional[str] = None
    ) -> None:
        """Method generated by attrs for class Assignment.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    id: typing.Optional[str]
    task_suite_id: typing.Optional[str]
    pool_id: typing.Optional[str]
    user_id: typing.Optional[str]
    status: typing.Optional[Status]
    reward: typing.Optional[decimal.Decimal]
    tasks: typing.Optional[typing.List[toloka.client.task.Task]]
    automerged: typing.Optional[bool]
    created: typing.Optional[datetime.datetime]
    submitted: typing.Optional[datetime.datetime]
    accepted: typing.Optional[datetime.datetime]
    rejected: typing.Optional[datetime.datetime]
    skipped: typing.Optional[datetime.datetime]
    expired: typing.Optional[datetime.datetime]
    first_declined_solution_attempt: typing.Optional[typing.List[toloka.client.solution.Solution]]
    solutions: typing.Optional[typing.List[toloka.client.solution.Solution]]
    mixed: typing.Optional[bool]
    public_comment: typing.Optional[str]


class AssignmentPatch(toloka.client.primitives.base.BaseTolokaObject):
    """Allows you to accept or reject tasks, and leave a comment

    Used in "TolokaClient.patch_assignment" method.

    Attributes:
        public_comment: Public comment about an assignment. Why it was accepted or rejected.
        status: Status of an assigned task suite.
            * ACCEPTED - Accepted by the requester.
            * REJECTED - Rejected by the requester.
    """

    def __init__(
        self,
        *,
        public_comment: typing.Optional[str] = None,
        status: typing.Optional[Assignment.Status] = None
    ) -> None:
        """Method generated by attrs for class AssignmentPatch.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    public_comment: typing.Optional[str]
    status: typing.Optional[Assignment.Status]


class GetAssignmentsTsvParameters(toloka.client.primitives.parameter.Parameters):
    """Allows you to downloads assignments as pandas.DataFrame

    Used in "TolokaClient.get_assignments_df" method.
    Implements the same behavior as if you download results in web-interface and then read it by pandas.

    Attributes:
        status: Assignments in which statuses will be downloaded.
        start_time_from: Upload assignments submitted after the specified date and time.
        start_time_to: Upload assignments submitted before the specified date and time.
        exclude_banned: Exclude answers from banned performers, even if assignments in suitable status "ACCEPTED".
        field: The names of the fields to be unloaded. Only the field names from the Assignment class, all other fields
            are added by default.
    """

    class Status(toloka.util._extendable_enum.ExtendableStrEnum):
        """An enumeration.
        """

        ACTIVE = 'ACTIVE'
        SUBMITTED = 'SUBMITTED'
        APPROVED = 'APPROVED'
        REJECTED = 'REJECTED'
        SKIPPED = 'SKIPPED'
        EXPIRED = 'EXPIRED'

    class Field(toloka.util._extendable_enum.ExtendableStrEnum):
        """An enumeration.
        """

        LINK = 'ASSIGNMENT:link'
        ASSIGNMENT_ID = 'ASSIGNMENT:assignment_id'
        TASK_ID = 'ASSIGNMENT:task_id'
        TASK_SUITE_ID = 'ASSIGNMENT:task_suite_id'
        WORKER_ID = 'ASSIGNMENT:worker_id'
        STATUS = 'ASSIGNMENT:status'
        STARTED = 'ASSIGNMENT:started'
        SUBMITTED = 'ASSIGNMENT:submitted'
        ACCEPTED = 'ASSIGNMENT:accepted'
        REJECTED = 'ASSIGNMENT:rejected'
        SKIPPED = 'ASSIGNMENT:skipped'
        EXPIRED = 'ASSIGNMENT:expired'
        REWARD = 'ASSIGNMENT:reward'

    @classmethod
    def structure(cls, data): ...

    def unstructure(self) -> dict: ...

    def __init__(
        self,
        *,
        status: typing.Optional[typing.List[Status]] = ...,
        start_time_from: typing.Optional[datetime.datetime] = None,
        start_time_to: typing.Optional[datetime.datetime] = None,
        exclude_banned: typing.Optional[bool] = None,
        field: typing.Optional[typing.List[Field]] = ...
    ) -> None:
        """Method generated by attrs for class GetAssignmentsTsvParameters.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    status: typing.Optional[typing.List[Status]]
    start_time_from: typing.Optional[datetime.datetime]
    start_time_to: typing.Optional[datetime.datetime]
    exclude_banned: typing.Optional[bool]
    field: typing.Optional[typing.List[Field]]
