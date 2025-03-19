import typing
import ujson as json
from random import shuffle

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupTranscriptType


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs

    with open(inputs.get('records_bits_s3_urls.json')) as f:
        bit_filename_to_s3_url: typing.Dict[str, str] = json.load(f)
    with open(inputs.get('recognitions.json')) as f:
        recognitions: typing.Dict[str, typing.List[str]] = json.load(f)

    # Check that most current recognitions format is used.
    assert all(isinstance(texts, list) for texts in recognitions.values())

    tasks = []
    for bit_filename, s3_url in bit_filename_to_s3_url.items():
        url = bit_filename_to_s3_url[bit_filename]
        text = recognitions[bit_filename][0]
        cls = MarkupTranscriptType.SPEECH if text else MarkupTranscriptType.NO_SPEECH
        tasks.append(
            {
                'input_values': {
                    'url': url,
                    'text': text,
                    'cls': cls.to_toloka_output(),
                },
            }
        )

    shuffle(tasks)

    with open(outputs.get('tasks.json'), 'w') as f:
        json.dump(tasks, f, ensure_ascii=False, indent=4)
