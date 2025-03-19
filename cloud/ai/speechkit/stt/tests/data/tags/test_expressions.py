import unittest

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTagType
from cloud.ai.speechkit.stt.lib.data.model.tags import (
    parse_tag_conjunctions,
    validate_tag_conjunctions,
    RecordTagDataRequest,
)
from cloud.ai.speechkit.stt.lib.data.ops.queries import tag_conjunctions_to_yql_query


class TestTagExpressions(unittest.TestCase):
    def test_parse_tag_conjunctions(self):
        self.assertEqual(
            [
                {
                    RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                    RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-03', negation=False),
                },
                {
                    RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                    RecordTagDataRequest(type=RecordTagType.MODE, value='short', negation=True),
                    RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-04', negation=False),
                },
                {
                    RecordTagDataRequest(type=RecordTagType.CLIENT, value='just-ai-tm2', negation=False),
                    RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=False),
                    RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                },
                {
                    RecordTagDataRequest(type=RecordTagType.MODE, value=None, negation=False),
                    RecordTagDataRequest(type=RecordTagType.CLIENT, value=None, negation=True),
                    RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-06', negation=False),
                },
            ],
            parse_tag_conjunctions(
                [
                    'IMPORT:yandex-call-center PERIOD:2020-03',
                    'CLIENT:mtt-u16 ~MODE:short PERIOD:2020-04',
                    'CLIENT:just-ai-tm2 MODE:stream PERIOD:2020-05',
                    'MODE ~CLIENT PERIOD:2020-06',
                ],
            ),
        )

        for invalid_tag_conjunction in [
            'CLIENT:mtt-u16 LOL PERIOD:2020-05',
            'CLIENT:mtt-u16 FOO:bar PERIOD:2020-05',
            'CLIENT:mtt-u16 +MODE PERIOD:2020-05',
        ]:
            with self.assertRaises(ValueError):
                parse_tag_conjunctions([invalid_tag_conjunction])

    def test_validate_tag_conjunctions(self):
        with self.assertRaises(ValueError):
            validate_tag_conjunctions(
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-03', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                ]
            )
        with self.assertRaises(ValueError):
            validate_tag_conjunctions(
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-03', negation=False),
                    },
                ]
            )

    def test_tag_conjunctions_to_yql_query_for_records(self):
        self.assertEqual(
            """

USE hahn;

PRAGMA yt.LookupJoinMaxRows = "500";  -- will slow down query, see YQL-12923

$tags_count_with_type = ($tags, $type) -> {
    RETURN ListLength(
        ListFilter(
            $tags,
            ($tag) -> {
                RETURN String::StartsWith($tag, $type)
            }
        )
    );
};

$records_with_tags = (
    SELECT
        records.`id` AS record_id,
        AGGREGATE_LIST(
            Yson::LookupString(records_tags.`data`, "type") || ":" || Yson::LookupString(records_tags.`data`, "value")
        ) AS tags
    FROM
        LIKE(`//home/mlcloud/speechkit/stt/records`, "%") AS records
    INNER JOIN
        LIKE(`//home/mlcloud/speechkit/stt/records_tags`, "%") AS records_tags
    ON
        records.`id` = records_tags.`record_id`
    GROUP BY
        records.`id`
);

$records_with_specified_tags = (
    SELECT
        *
    FROM
        $records_with_tags AS records_with_tags
WHERE
ListHas(records_with_tags.`tags`, "CLIENT:mtt-u16") AND ListHas(records_with_tags.`tags`, "MODE:stream") AND ListHas(records_with_tags.`tags`, "PERIOD:2020-05")
OR
ListHas(records_with_tags.`tags`, "IMPORT:yandex-call-center") AND ListHas(records_with_tags.`tags`, "PERIOD:2020-03")
OR
ListHas(records_with_tags.`tags`, "CLIENT:just-ai-tm2") AND NOT ListHas(records_with_tags.`tags`, "MODE:stream") AND ListHas(records_with_tags.`tags`, "PERIOD:2020-05")
OR
$tags_count_with_type(records_with_tags.`tags`, "CLIENT") = 0 AND $tags_count_with_type(records_with_tags.`tags`, "MODE") > 0 AND ListHas(records_with_tags.`tags`, "PERIOD:2020-06")
);
""",
            tag_conjunctions_to_yql_query(
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-03', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='just-ai-tm2', negation=False),
                        RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=True),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.MODE, value=None, negation=False),
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value=None, negation=True),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-06', negation=False),
                    },
                ],
                'hahn',
            ),
        )

    def test_tag_conjunctions_to_yql_query_for_records_bits(self):
        self.assertEqual(
            """

USE hahn;

PRAGMA yt.LookupJoinMaxRows = "500";  -- will slow down query, see YQL-12923

$tags_count_with_type = ($tags, $type) -> {
    RETURN ListLength(
        ListFilter(
            $tags,
            ($tag) -> {
                RETURN String::StartsWith($tag, $type)
            }
        )
    );
};

$records_with_tags = (
    SELECT
        records.`id` AS record_id,
        AGGREGATE_LIST(
            Yson::LookupString(records_tags.`data`, "type") || ":" || Yson::LookupString(records_tags.`data`, "value")
        ) AS tags
    FROM
        LIKE(`//home/mlcloud/speechkit/stt/records`, "%") AS records
    INNER JOIN
        LIKE(`//home/mlcloud/speechkit/stt/records_tags`, "%") AS records_tags
    ON
        records.`id` = records_tags.`record_id`
    GROUP BY
        records.`id`
);

$records_bits_with_records_tags = (
    SELECT
        records_bits.`id` AS record_bit_id,
        records_with_tags.`tags` AS tags
    FROM
        $records_with_tags AS records_with_tags
    INNER JOIN
        LIKE(`//home/mlcloud/speechkit/stt/records_bits`, "%") AS records_bits
    ON
        records_with_tags.`record_id` = records_bits.`record_id`
);

$records_bits_with_records_bits_tags = (
    SELECT
        records_bits.`id` AS record_bit_id,
        AGGREGATE_LIST(
            Yson::LookupString(records_tags.`data`, "type") || ":" || Yson::LookupString(records_tags.`data`, "value")
        ) AS tags
    FROM
        LIKE(`//home/mlcloud/speechkit/stt/records_bits`, "%") AS records_bits
    INNER JOIN
        LIKE(`//home/mlcloud/speechkit/stt/records_tags`, "%") AS records_tags
    ON
        records_bits.`id` = records_tags.`record_id`
    GROUP BY
        records_bits.`id`
);

$records_bits_with_tags = (
    SELECT
        IF(
            records_bits_with_records_tags.`record_bit_id` IS NULL,
            records_bits_with_records_bits_tags.`record_bit_id`,
            records_bits_with_records_tags.`record_bit_id`
        ) AS record_id,
        ListUniq(ListExtend(
            IF(
                records_bits_with_records_tags.`tags` IS NULL,
                [],
                records_bits_with_records_tags.`tags`
            ),
            IF(
                records_bits_with_records_bits_tags.`tags` IS NULL,
                [],
                records_bits_with_records_bits_tags.`tags`
            )
        )) AS tags
    FROM
        $records_bits_with_records_tags AS records_bits_with_records_tags
    FULL JOIN
        $records_bits_with_records_bits_tags AS records_bits_with_records_bits_tags
    ON
        records_bits_with_records_tags.`record_bit_id` = records_bits_with_records_bits_tags.`record_bit_id`
);

$records_with_specified_tags = (
    SELECT
        *
    FROM
        $records_bits_with_tags AS records_bits_with_tags
WHERE
ListHas(records_bits_with_tags.`tags`, "CLIENT:mtt-u16") AND ListHas(records_bits_with_tags.`tags`, "MODE:stream") AND ListHas(records_bits_with_tags.`tags`, "PERIOD:2020-05")
OR
ListHas(records_bits_with_tags.`tags`, "IMPORT:yandex-call-center") AND ListHas(records_bits_with_tags.`tags`, "PERIOD:2020-03")
OR
ListHas(records_bits_with_tags.`tags`, "CLIENT:just-ai-tm2") AND NOT ListHas(records_bits_with_tags.`tags`, "MODE:stream") AND ListHas(records_bits_with_tags.`tags`, "PERIOD:2020-05")
OR
$tags_count_with_type(records_bits_with_tags.`tags`, "CLIENT") = 0 AND $tags_count_with_type(records_bits_with_tags.`tags`, "MODE") > 0 AND ListHas(records_bits_with_tags.`tags`, "PERIOD:2020-06")
);
""",
            tag_conjunctions_to_yql_query(
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-03', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='just-ai-tm2', negation=False),
                        RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=True),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.MODE, value=None, negation=False),
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value=None, negation=True),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-06', negation=False),
                    },
                ],
                'hahn',
                bits=True,
            ),
        )
