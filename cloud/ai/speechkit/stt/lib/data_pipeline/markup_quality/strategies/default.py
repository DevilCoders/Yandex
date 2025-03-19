from enum import Enum

from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupSolution, SolutionFieldDiff, SolutionAcceptanceResult


def default_honeypot_acceptance_strategy(
    expected_solution: MarkupSolution,
    actual_solution: MarkupSolution,
) -> SolutionAcceptanceResult:
    expected_attr_names = expected_solution.__dict__.keys()
    actual_attr_names = actual_solution.__dict__.keys()

    assert expected_attr_names == actual_attr_names

    diff_list = []
    for attr_name in expected_attr_names:
        expected_attr = getattr(expected_solution, attr_name)
        actual_attr = getattr(actual_solution, attr_name)
        assert type(expected_attr) == type(actual_attr), 'solution attrs sets must be equal'
        assert any(
            isinstance(expected_attr, t)
            for t in [str, int, bool, Enum]
        ), 'unsupported solution attr type'
        if isinstance(expected_attr, Enum):
            expected_attr = expected_attr.value
            actual_attr = actual_attr.value
        if expected_attr != actual_attr:
            diff_list.append(SolutionFieldDiff(
                field_name=attr_name,
                expected_value=expected_attr,
                actual_value=actual_attr,
                extra=None,
            ))

    return SolutionAcceptanceResult(
        accepted=len(diff_list) == 0,
        diff_list=diff_list,
    )
