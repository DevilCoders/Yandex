import os
import soundfile as snd

from copy import deepcopy
from io import BytesIO

from tools.audio import AudioSample
from tools.data_interface.synthesis import SynthesisSample, read_synthesized_samples, save_synthesized_samples


def gsm_codec(audio_sample):
    buffer = BytesIO()
    snd.write(buffer, audio_sample.data, audio_sample.sample_rate, subtype='GSM610', format='wav')
    buffer.seek(0)
    data, sample_rate = snd.read(buffer, dtype='float32')
    gsm_audio_sample = AudioSample(data=data, sample_rate=sample_rate)
    return gsm_audio_sample


def add_noise_in_pairs(audios, references):
    noised_audios = list(map(lambda audio_sample: gsm_codec(audio_sample), audios))

    def get_noised_reference(reference):
        noised_copy = deepcopy(reference)
        noised_copy.audio = gsm_codec(reference.audio)
        return noised_copy

    noised_references = list(map(lambda reference_sample: get_noised_reference(reference_sample), references))

    return noised_audios, noised_references


def add_noise(samples):
    new_samples = []

    for sample in samples:
        new_reference = deepcopy(sample.reference)
        new_reference.audio = gsm_codec(new_reference.audio)

        synthesis_audio = deepcopy(sample.synthesis_audio)
        synthesis_audio = gsm_codec(synthesis_audio)

        new_sample = SynthesisSample(new_reference, sample.synthesized_text, synthesis_audio)
        new_samples.append(new_sample)

    return new_samples


def noise_every_iter(src_path, dst_path):
    for iteration in os.listdir(src_path):
        path_samples = os.path.join(src_path, iteration)
        samples = read_synthesized_samples(path_samples)

        samples_noised = add_noise(samples)

        path_noised_samples = os.path.join(dst_path, iteration)

        if not os.path.exists(path_noised_samples):
            os.makedirs(path_noised_samples)
        else:
            print(f'Path {path_noised_samples} already exists, skipping...')
            continue

        save_synthesized_samples(path_noised_samples, samples_noised)

