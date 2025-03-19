from collections import defaultdict
from datetime import datetime, timedelta, timezone
import requests

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.dao import MetricEvalTag, RecognitionEndpoint
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_metrics_eval_tags_meta

tag_map = {
    'default': 'EXEMPLAR:primal-30',
    'tmp:testamentum-30': 'EXEMPLAR:primal-30',
    'EXEMPLAR:primal-30': 'EXEMPLAR:primal-30',
    'OTHER:call-center-288-test-2020-01': 'OTHER:call-center-288-test-2020-01',
    'CLIENT-PERIOD:neuro-net-md8_2020-04': 'CLIENT-PERIOD:neuro-net-md8_2020-04',
}

metric_map = {
    'WER': {
        'metric_name': 'WER',
        'text_transformations': '',
    },
    'WER_norm': {
        'metric_name': 'WER',
        'text_transformations': 'norm',
    },
    'LER': {
        'metric_name': 'WER',
        'text_transformations': 'lemm',
    },
    'LER_norm': {
        'metric_name': 'WER',
        'text_transformations': 'norm_lemm',
    },
}

api_map = {
    'google': {
        'api': 'google',
        'model': '',
        'recognition_endpoint': {
            "api": "google",
            "config": {
                "config": {},
                "interim_results": True,
            },
            "host": "speech.googleapis.com",
            "method": {"name": "stream", "version": "v1"},
            "port": 443,
        },
    },
    'googlePhone': {
        'api': 'google',
        'model': 'phone_call',
        'recognition_endpoint': {
            "api": "google",
            "config": {"config": {"model": "phone_call", "use_enhanced": True}, "interim_results": True},
            "host": "speech.googleapis.com",
            "method": {"name": "stream", "version": "v1"},
            "port": 443,
        },
    },
    'yandex': {
        'api': 'yandex',
        'model': 'general',
        'recognition_endpoint': {
            "api": "yandex",
            "config": {"specification": {"model": "general", "partial_results": True}},
            "host": "stt.api.cloud.yandex.net",
            "method": {"name": "stream", "version": "v1"},
            "port": 443,
        },
    },
    'yandexGeneralRC': {
        'api': 'yandex',
        'model': 'general:rc',
        'recognition_endpoint': {
            "api": "yandex",
            "config": {"specification": {"model": "general:rc", "partial_results": True}},
            "host": "stt.api.cloud.yandex.net",
            "method": {"name": "stream", "version": "v1"},
            "port": 443,
        },
    },
    'tinkoff': {
        'api': 'tinkoff',
        'model': '',
        'recognition_endpoint': {
            "api": "tinkoff",
            "config": {"config": {"Vad": None}, "interim_results_config": {"enable_interim_results": True}},
            "host": "stt.tinkoff.ru",
            "method": {"name": "stream", "version": "v1"},
            "port": 443,
        },
    },
}


def main():
    metrics = []
    for metrics_tag in tag_map.keys():
        for metrics_api in api_map.keys():
            for metrics_metric in metric_map.keys():
                resp = requests.post(
                    'https://metrics.yandex-team.ru/'
                    'api-history/graph/lines?limit=5000&withoutComments=true&withoutProperties=true',
                    json={
                        'startDate': '2019-01-01T11:24:59+03:00',
                        'endDate': '2020-07-07T00:00:00+03:00',
                        'pointsAggregateType': 'CRON_EXPRESSION',
                        'config': [
                            {
                                'cronId': '102295',
                                'filter': {
                                    'system': metrics_api,
                                    'filter': metrics_tag,
                                    'metric': metrics_metric,
                                },
                            }
                        ],
                    },
                ).json()[0]

                tag = tag_map[metrics_tag]

                api = api_map[metrics_api]['api']
                model = api_map[metrics_api]['model']
                recognition_endpoint = RecognitionEndpoint.from_yson(api_map[metrics_api]['recognition_endpoint'])

                metric_name = metric_map[metrics_metric]['metric_name']
                text_transformations = metric_map[metrics_metric]['text_transformations']

                for point in resp['points']:
                    timestamp = point['timestamp']  # example: "2019-12-17T04:15:47.000+03:00"
                    if timestamp is None:
                        continue

                    received_at = (datetime.fromisoformat(timestamp) + timedelta(seconds=0.000001)).astimezone(
                        timezone.utc
                    )

                    value = point['value'] / 100

                    metrics.append(
                        MetricEvalTag(
                            tag=tag,
                            metric_name=metric_name,
                            aggregation='mean',
                            text_transformations=text_transformations,
                            api=api,
                            model=model,
                            method='stream',
                            eval_id=generate_id(),
                            metric_value=value,
                            recognition_endpoint=recognition_endpoint,
                            received_at=received_at,
                            other=None,
                        )
                    )

    table_name_to_metrics = defaultdict(list)
    for metric in metrics:
        table_name_to_metrics[Table.get_name(metric.received_at)].append(metric)

    for table_name, metrics in table_name_to_metrics.items():
        table = Table(table_metrics_eval_tags_meta, table_name)
        table.append_objects(metrics)
