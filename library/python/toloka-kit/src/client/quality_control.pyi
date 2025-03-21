__all__ = [
    'QualityControl',
]
import toloka.client.actions
import toloka.client.collectors
import toloka.client.conditions
import toloka.client.primitives.base
import toloka.client.task_distribution_function
import toloka.util._extendable_enum
import typing


class QualityControl(toloka.client.primitives.base.BaseTolokaObject):
    """Quality control unit settings and pool ID with training tasks

    Quality control lets you get more accurate responses and restrict access to tasks for cheating performers.
    Quality control consists of rules. All rules work independently.

    Attributes:
        training_requirement: Parameters of the training pool that is linked to the pool with the main tasks.
        captcha_frequency: Frequency of captcha display (By default, captcha is not shown):
            LOW - show every 20 tasks.
            MEDIUM, HIGH - show every 10 tasks.
        configs: List of quality control units. See QualityControl.QualityControlConfig
        checkpoints_config: Random check majority opinion. Datailed description in QualityControl.CheckpointsConfig.

    Example:
        How to set up quality control on new pool.

        >>> new_pool = toloka.pool.Pool(....)
        >>> new_pool.quality_control.add_action(
        >>>     collector=toloka.collectors.AssignmentSubmitTime(history_size=5, fast_submit_threshold_seconds=20),
        >>>     conditions=[toloka.conditions.FastSubmittedCount > 1],
        >>>     action=toloka.actions.RestrictionV2(
        >>>         scope=toloka.user_restriction.UserRestriction.ALL_PROJECTS,
        >>>         duration=10,
        >>>         duration_unit='DAYS',
        >>>         private_comment='Fast responses',  # Only you will see this comment
        >>>     )
        >>> )
        ...
    """

    class TrainingRequirement(toloka.client.primitives.base.BaseTolokaObject):
        """Parameters of the training pool that is linked to the pool with the main tasks

        Attributes:
            training_pool_id: ID of the training pool that is linked to the pool with the main tasks.
            training_passing_skill_value: The percentage of correct answers in training tasks (from 0 to 100) required
                for admission to the main tasks. The user's first responses in tasks are used for counting.
        """

        def __init__(
            self,
            *,
            training_pool_id: typing.Optional[str] = None,
            training_passing_skill_value: typing.Optional[int] = None
        ) -> None:
            """Method generated by attrs for class QualityControl.TrainingRequirement.
            """
            ...

        _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
        training_pool_id: typing.Optional[str]
        training_passing_skill_value: typing.Optional[int]

    class CaptchaFrequency(toloka.util._extendable_enum.ExtendableStrEnum):
        """An enumeration.
        """

        LOW = 'LOW'
        MEDIUM = 'MEDIUM'
        HIGH = 'HIGH'

    class CheckpointsConfig(toloka.client.primitives.base.BaseTolokaObject):
        """Random check majority opinion.

        Only some tasks are issued with a high overlap (for example, "5") and are being tested.
        Other tasks are issued with the overlap set in the pool settings (for example, "1") and remain without verification.
        Spot check saves money and speeds up pool execution.

        You can reduce the frequency of checks over time.

        Example settings: in the first 25 tasks completed by the user in the pool, issue every fifth task with an overlap "5"
        to check the answers. In subsequent tasks issue each 25 task with an overlap "5".

        Attributes:
            real_settings: Checkpoints settings for main tasks.
            golden_settings: Checkpoints settings for golden tasks.
            training_settings: Checkpoints settings for train tasks.
        """

        class Settings(toloka.client.primitives.base.BaseTolokaObject):
            """Setting for checkpoints

            Attributes:
                target_overlap: Overlap in tasks with majority opinion verification.
                task_distribution_function: Distribution of tasks with majority opinion verification.
            """

            def __init__(
                self,
                *,
                target_overlap: typing.Optional[int] = None,
                task_distribution_function: typing.Optional[toloka.client.task_distribution_function.TaskDistributionFunction] = None
            ) -> None:
                """Method generated by attrs for class QualityControl.CheckpointsConfig.Settings.
                """
                ...

            _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
            target_overlap: typing.Optional[int]
            task_distribution_function: typing.Optional[toloka.client.task_distribution_function.TaskDistributionFunction]

        def __init__(
            self,
            *,
            real_settings: typing.Optional[Settings] = None,
            golden_settings: typing.Optional[Settings] = None,
            training_settings: typing.Optional[Settings] = None
        ) -> None:
            """Method generated by attrs for class QualityControl.CheckpointsConfig.
            """
            ...

        _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
        real_settings: typing.Optional[Settings]
        golden_settings: typing.Optional[Settings]
        training_settings: typing.Optional[Settings]

    class QualityControlConfig(toloka.client.primitives.base.BaseTolokaObject):
        """Quality control block

        Quality control blocks help regulate access to a project or pool: you can filter out users who give incorrect answers
        in control tasks, skip many tasks in a row, and so on.

        The block consists of two parts: condition and the action to be performed when the condition is met.
        There may be several conditions, then they are combined using logical And.

        Attributes:
            rules: Conditions and action if conditions are met.
            collector_config: Parameters for collecting statistics (for example, the number of task skips in the pool).
        """

        class RuleConfig(toloka.client.primitives.base.BaseTolokaObject):
            """Conditions and action if conditions are met

            The values for the conditions are taken from the collector.

            Attributes:
                action: Action if conditions are met (for example, close access to the project).
                conditions: Conditions (for example, skipping 10 sets of tasks in a row).
            """

            def __init__(
                self,
                *,
                action: typing.Optional[toloka.client.actions.RuleAction] = None,
                conditions: typing.Optional[typing.List[toloka.client.conditions.RuleCondition]] = None
            ) -> None:
                """Method generated by attrs for class QualityControl.QualityControlConfig.RuleConfig.
                """
                ...

            _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
            action: typing.Optional[toloka.client.actions.RuleAction]
            conditions: typing.Optional[typing.List[toloka.client.conditions.RuleCondition]]

        def __init__(
            self,
            *,
            rules: typing.Optional[typing.List[RuleConfig]] = None,
            collector_config: typing.Optional[toloka.client.collectors.CollectorConfig] = None
        ) -> None:
            """Method generated by attrs for class QualityControl.QualityControlConfig.
            """
            ...

        _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
        rules: typing.Optional[typing.List[RuleConfig]]
        collector_config: typing.Optional[toloka.client.collectors.CollectorConfig]

    def add_action(
        self,
        collector: toloka.client.collectors.CollectorConfig,
        action: toloka.client.actions.RuleAction,
        conditions: typing.List[toloka.client.conditions.RuleCondition]
    ):
        """Adds new QualityControlConfig to QualityControl object. Usually in pool.

        See example in QualityControl class.

        Arg:
            collector: Parameters for collecting statistics (for example, the number of task skips in the pool).
            action: Action if conditions are met (for example, close access to the project).
            conditions: Conditions (for example, skipping 10 sets of tasks in a row).
        """
        ...

    def __init__(
        self,
        *,
        training_requirement: typing.Optional[TrainingRequirement] = None,
        captcha_frequency: typing.Union[CaptchaFrequency, str, None] = None,
        configs: typing.Optional[typing.List[QualityControlConfig]] = ...,
        checkpoints_config: typing.Optional[CheckpointsConfig] = None
    ) -> None:
        """Method generated by attrs for class QualityControl.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    training_requirement: typing.Optional[TrainingRequirement]
    captcha_frequency: typing.Optional[CaptchaFrequency]
    configs: typing.Optional[typing.List[QualityControlConfig]]
    checkpoints_config: typing.Optional[CheckpointsConfig]
