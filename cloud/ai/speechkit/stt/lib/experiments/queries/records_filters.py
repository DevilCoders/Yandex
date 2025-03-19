from typing import Iterable
from cloud.ai.speechkit.stt.lib.data.model.dao import Mark


def get_marks_filter(marks: Iterable[Mark]):
    marks_filter = ' OR '.join(f"records.`mark` = '{mark}'" for mark in marks)
    return marks_filter
