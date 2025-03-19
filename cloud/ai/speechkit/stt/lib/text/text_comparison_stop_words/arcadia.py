import json
import os
import typing

from cloud.ai.speechkit.stt.lib.utils.arcadia import arcadia_download

stop_words_arcadia_dir_path = 'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words'


def get_text_comparison_stop_words_from_arcadia(
    topic: str,
    lang: str,
    revision: int,
    arcadia_token: str,
) -> typing.List[str]:
    stop_words = arcadia_download(
        path=os.path.join(stop_words_arcadia_dir_path, lang, f'{topic}.json'),
        revision=revision,
        token=arcadia_token,
    )
    return json.loads(stop_words)
