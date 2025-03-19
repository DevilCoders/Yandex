import os
import typing
from dataclasses import dataclass

from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupDataVersions, MarkupAssignmentStatus, MarkupTranscriptType
from .select import run_yql_select_query
from ..yt import table_records_bits_markups_meta, table_markups_assignments_meta, table_metrics_markup_meta


@dataclass
class TranscriptHoneypotCandidateRow:
    audio_s3_key: str
    texts: typing.List[str]

    @staticmethod
    def from_yson(fields: dict) -> 'TranscriptHoneypotCandidateRow':
        return TranscriptHoneypotCandidateRow(
            audio_s3_key=fields['audio_s3_key'],
            texts=fields['texts'],
        )


def select_transcript_honeypots_candidates(
    markups_table_name_ge: typing.Optional[str],
    random_sample_fraction: float,
    text_min_length: int,
    text_min_words: int,
    solution_min_overlap: int,
    max_unique_texts: int,
    from_honeypots: bool,
    lang: str,
) -> typing.List[TranscriptHoneypotCandidateRow]:
    markup_table_range_args = f', "{markups_table_name_ge}"' if markups_table_name_ge else ''
    known_solutions_operator = '>' if from_honeypots else '='

    yql_query = f"""
USE {os.environ['YT_PROXY']};

USE hahn;

$all_tasks_lists = (
SELECT
    Yson::ConvertToList(`tasks`) AS tasks
FROM
    RANGE(`{table_markups_assignments_meta.dir_path}`{markup_table_range_args}) AS markups_assignments
INNER JOIN
    RANGE(`{table_metrics_markup_meta.dir_path}`{markup_table_range_args}) AS metrics
ON
    markups_assignments.markup_id = metrics.markup_id
WHERE
    metrics.language = '{lang}' AND
    Yson::LookupString(`data`, 'status') = '{MarkupAssignmentStatus.ACCEPTED.value}'
);

$all_tasks = (
SELECT
    *
FROM
    $all_tasks_lists
FLATTEN BY
    tasks AS task
);

$tasks = (
    SELECT
        Yson::LookupString(Yson::Lookup(Yson::Lookup(task, "input"), "audio_s3_obj"), "key") AS audio_s3_key,
        Yson::LookupString(Yson::Lookup(task, "solution"), "text") AS text,
        Yson::LookupString(Yson::Lookup(task, "solution"), "type") AS type
    FROM
        $all_tasks
    WHERE
        ListLength(Yson::LookupList(task, "known_solutions")) {known_solutions_operator} 0 AND
        Yson::LookupString(task, "version") = "{MarkupDataVersions.TRANSCRIPT_AND_TYPE.value}"
);

-- Filter on text length and words count
$markup_text_filter = Python3::filter_text(
    Callable<(String?)->Bool>,
@@
def filter_text(text):
    text = text.decode('utf-8')
    return len(text) >= {text_min_length} and len(text.split(' ')) >= {text_min_words}
@@
);

$text_filtered_tasks = (
    SELECT
        audio_s3_key,
        text
    FROM $tasks
    WHERE
        type = '{MarkupTranscriptType.SPEECH.value}' AND
        $markup_text_filter(text) = TRUE
);

$audio_total_solutions = (
    SELECT
       audio_s3_key, COUNT(*) AS cnt
    FROM
        $text_filtered_tasks
    GROUP BY
        audio_s3_key
);

$audio_particular_text_solutions = (
    SELECT
        audio_s3_key, text, COUNT(*) AS cnt
    FROM
        $text_filtered_tasks
    GROUP BY
        audio_s3_key, text
);

$audio_unique_text_solutions = (
    SELECT
        audio_s3_key, COUNT(*) AS cnt
    FROM
        $audio_particular_text_solutions
    GROUP BY
        audio_s3_key
);

$audio_texts = (
    SELECT
        particular_text_solutions.audio_s3_key AS audio_s3_key,
        particular_text_solutions.text AS text,
        total_solutions.cnt AS total_count,
        unique_text_solutions.cnt AS unique_text_count,
        particular_text_solutions.cnt AS particular_text_count
    FROM
        $audio_particular_text_solutions AS particular_text_solutions
    INNER JOIN
        $audio_total_solutions AS total_solutions
    ON
        particular_text_solutions.audio_s3_key = total_solutions.audio_s3_key
    INNER JOIN
        $audio_unique_text_solutions AS unique_text_solutions
    ON
        particular_text_solutions.audio_s3_key = unique_text_solutions.audio_s3_key
);

$filtered_audio_texts = (
    SELECT
        *
    FROM
        $audio_texts
    WHERE
        total_count >= {solution_min_overlap} AND
        unique_text_count <= {max_unique_texts} AND
        CAST(particular_text_count AS double) > Math::Sqrt(CAST(total_count AS double))
);

$honeypots_candidates = (
    SELECT
        audio_s3_key, AGGREGATE_LIST(text) AS texts
    FROM
        $filtered_audio_texts
    GROUP BY
        audio_s3_key
);

-- Take random sample from candidates table
SELECT
    *
FROM
    $honeypots_candidates
TABLESAMPLE BERNOULLI({random_sample_fraction * 100.0})
ORDER BY
    Digest::Crc64(`audio_s3_key` || CAST(RandomUuid(`audio_s3_key`) AS string));  -- Random shuffle
"""

    honeypots_candidates = []
    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    for row in rows:
        honeypots_candidates.append(TranscriptHoneypotCandidateRow.from_yson(row))
    return honeypots_candidates


@dataclass
class CheckTranscriptHoneypotCandidateRow:
    audio_s3_key: str
    text: str
    ok: bool
    type: MarkupTranscriptType

    @staticmethod
    def from_yson(fields: dict) -> 'CheckTranscriptHoneypotCandidateRow':
        return CheckTranscriptHoneypotCandidateRow(
            audio_s3_key=fields['audio_s3_key'],
            text=fields['text'],
            ok=bool(fields['ok']),
            type=MarkupTranscriptType(fields['type']),
        )


def select_check_transcript_honeypots_candidates(
    markups_table_name_ge: typing.Optional[str],
    random_sample_fraction: float,
    solution_min_overlap: int,
    text_min_length: int,
    text_min_words: int,
    lang: str,
) -> typing.List[CheckTranscriptHoneypotCandidateRow]:
    markup_table_range_args = f', "{markups_table_name_ge}"' if markups_table_name_ge else ''
    yql_query = f"""
USE {os.environ['YT_PROXY']};

-- No honeypots here
$markups = (
    SELECT
        Yson::LookupString(Yson::Lookup(Yson::Lookup(markups.`markup_data`, "input"), "audio_s3_obj"), "key") AS audio_s3_key,
        Yson::LookupString(Yson::Lookup(markups.`markup_data`, "input"), "text") AS text,
        Yson::LookupBool(Yson::Lookup(markups.`markup_data`, "solution"), "ok") AS ok,
        Yson::LookupString(Yson::Lookup(markups.`markup_data`, "solution"), "type") AS type
    FROM
        RANGE(`{table_records_bits_markups_meta.dir_path}`{markup_table_range_args}) AS markups
    INNER JOIN
        RANGE(`{table_metrics_markup_meta.dir_path}`{markup_table_range_args}) AS metrics
    ON
        markups.markup_id = metrics.markup_id
    WHERE
        metrics.language = '{lang}' AND
        Yson::LookupString(markups.`markup_data`, "version") = "{MarkupDataVersions.CHECK_TRANSCRIPT.value}"
);

-- Filter on text length and words count
$markup_text_filter_script = @@
def filter_text(text):
    text = text.decode('utf-8')
    return len(text) == 0 or len(text) >= {text_min_length} and len(text.split(' ')) >= {text_min_words}
@@;

$markup_text_filter = Python3::filter_text(
    Callable<(String?)->Bool>,
    $markup_text_filter_script
);

$filtered_markups = (
    SELECT
        audio_s3_key,
        text,
        ok,
        type
    FROM $markups
    WHERE
        $markup_text_filter(text) = TRUE
);


-- Count overlap and unique solutions
$grouped_markups = (
    SELECT
        audio_s3_key,
        COUNT(*) AS overall_markups_solutions,
        COUNT(DISTINCT AsTuple(ok, type)) AS unique_markups_solutions
    FROM $filtered_markups
    GROUP BY audio_s3_key
);

-- Get bits with overlap where all users write identical solution.
$matched_markups = (
    SELECT
        audio_s3_key,
        overall_markups_solutions,
        unique_markups_solutions
    FROM
        $grouped_markups
    WHERE
        overall_markups_solutions >= {solution_min_overlap} AND
        unique_markups_solutions = 1
);

-- Join solutions to audio IDs. There will be multiple rows
-- for each audio ID, but solutions will be equal due to
-- previous queries.
$honeypots_candidates = (
    SELECT
        matched_markups.`audio_s3_key` AS audio_s3_key,
        markups.`text` AS text,
        markups.`ok` AS ok,
        markups.`type` AS type
    FROM
        $matched_markups AS matched_markups
    LEFT JOIN
        $filtered_markups AS markups
    ON
        matched_markups.`audio_s3_key` = markups.`audio_s3_key`
);

-- Take random sample from candidates table
SELECT
    *
FROM
    $honeypots_candidates
TABLESAMPLE BERNOULLI({random_sample_fraction * 100.0})
ORDER BY
    Digest::Crc64(`audio_s3_key` || CAST(RandomUuid(`audio_s3_key`) AS string));  -- Random shuffle
"""

    honeypots_candidates = []
    received_audio_s3_keys = set([])  # for deduplication because of JOIN
    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    for row in rows:
        hc = CheckTranscriptHoneypotCandidateRow.from_yson(row)
        if hc.audio_s3_key in received_audio_s3_keys:
            continue
        received_audio_s3_keys.add(hc.audio_s3_key)
        honeypots_candidates.append(hc)
    return honeypots_candidates
