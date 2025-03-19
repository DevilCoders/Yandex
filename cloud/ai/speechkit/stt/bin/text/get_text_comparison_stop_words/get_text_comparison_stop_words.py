import json

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.dao import TextComparisonStopWordsArcadiaSource
from cloud.ai.speechkit.stt.lib.text.text_comparison_stop_words import get_text_comparison_stop_words_from_arcadia


def main():
    op_ctx = nv.context()
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    topic = params.get('topic')
    lang = params.get('lang')
    revision = params.get('revision')
    arcadia_token = params.get('arcadia-token')

    stop_words = get_text_comparison_stop_words_from_arcadia(topic, lang, revision, arcadia_token)

    with open(outputs.get('stop_words.json'), 'w') as f:
        json.dump(stop_words, f, indent=4, ensure_ascii=False)

    with open(outputs.get('info.json'), 'w') as f:
        json.dump(TextComparisonStopWordsArcadiaSource(
            topic=topic,
            lang=lang,
            revision=revision,
        ).to_yson(), f, indent=4, ensure_ascii=False)
