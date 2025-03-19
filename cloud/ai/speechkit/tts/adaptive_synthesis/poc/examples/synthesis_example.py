import sys
sys.path.append('..')
from argparse import ArgumentParser
from methods.tacotron_model import SynthesisModel
from tools.data import sample_sox
from tools.audio import AudioSample


def synthesize(
        path_to_vocoder,
        path_to_tacotron,
        path_to_lingware,
        path_to_preprocessor,
        text,
        path_to_reference_audio,
        speaker_id
):
    model = SynthesisModel(
        waveglow_path=path_to_vocoder,
        tacotron_path=path_to_tacotron,
        lingware_path=path_to_lingware,
        preprocessor_cli_path=path_to_preprocessor
    )

    audio = AudioSample(path_to_reference_audio)
    synthesized_audio = model.apply(text, audio, speaker_id=speaker_id)
    return synthesized_audio


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument('--vocoder', required=True)
    parser.add_argument('--tacotron', required=True)
    parser.add_argument('--lingware', required=True)
    parser.add_argument('--preprocessor', required=True)
    parser.add_argument('--text', required=True)
    parser.add_argument('--ref-audio', required=True)
    parser.add_argument('--speaker-id', default='elena_mtt_female')
    parser.add_argument('--output', default='synthesis.wav')
    default_sample_rate = 22050
    parser.add_argument('--output-sample-rate', default=default_sample_rate)
    params = parser.parse_args()

    audio = synthesize(
        params.vocoder,
        params.tacotron,
        params.lingware,
        params.preprocessor,
        params.text,
        params.ref_audio,
        params.speaker_id
    )
    if params.output_sample_rate != default_sample_rate:
        audio = sample_sox([audio], params.output_sample_rate)[0]
    audio.save(params.output)
