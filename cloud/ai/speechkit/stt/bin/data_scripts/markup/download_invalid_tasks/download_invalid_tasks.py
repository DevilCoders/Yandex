import argparse
import json

from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupStep,
    RecognitionEndpoint,
    RecordSourceCloudMethod,
    AudioURLAndTranscriptInput,
    AudioURLInput,
)


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Download rejected tasks given Toloka pool id and oAuth token',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        '--pool_id',
        required=True,
    )
    parser.add_argument('--markup_step', choices=(MarkupStep.TRANSCRIPT.value, MarkupStep.CHECK_ASR_TRANSCRIPT.value))
    parser.add_argument(
        '--oauth_token',
        required=True,
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def run(pool_id: str, markup_step: str, oauth_token: str):
    markup_step = MarkupStep(markup_step)

    markup_args = {}
    if markup_step == MarkupStep.CHECK_TRANSCRIPT:
        markup_args['recognition_params'] = RecognitionEndpoint(
            api='nevermind',
            host='nevermind',
            port=443,
            method=RecordSourceCloudMethod(name='nevermind', version='0'),
            config={},
        ).to_yson()

    assignments = TolokaClient(oauth_token=oauth_token, lang='ru-RU').get_assignments(
        pool_id, 'nevermind', markup_step, **markup_args
    )

    invalid_tasks = []
    for assignment in assignments:
        for task in assignment.tasks:
            if len(task.known_solutions) > 0 and task.known_solutions[0].solution != task.solution:
                if isinstance(task.input, AudioURLInput):
                    inp = {'audio_url': task.input.audio_s3_obj.to_https_url()}
                elif isinstance(task.input, AudioURLAndTranscriptInput):
                    inp = {
                        'audio_url': task.input.audio_s3_obj.to_https_url(),
                        'text': task.input.text,
                    }
                else:
                    raise ValueError(f'Not supported task input: {task.input}')
                invalid_tasks.append(
                    {
                        'input': inp,
                        'solution': task.solution.to_yson(),
                        'known_solution': task.known_solutions[0].solution.to_yson(),
                        'assignment_id': assignment.data.source_id,
                    }
                )
    with open(f'invalid_tasks_{pool_id}.json', 'w') as f:
        json.dump(invalid_tasks, f, indent=4, ensure_ascii=False)
