import os
from typing import List

from contrib.tts.tacotron.text_processing import symbols_to_ids
from methods.tacotron_model import SynthesisModel
from tools.data_interface.train import TrainingSample
from tools.data_interface.synthesis import SynthesisSample, save_synthesized_samples


def synthesize_on_train_sequeces(
    training_samples: List[TrainingSample],
    path_to_tacotron=None,
    path_to_waveglow=None,
    speaker_id=None
):
    model = SynthesisModel(tacotron_path=path_to_tacotron, waveglow_path=path_to_waveglow)
    synthesis_samples = []

    for sample in training_samples:
        sequence_ids = symbols_to_ids(['@BOS'] + sample.sequence + ['@EOS'])
        synthesized_audio = model.synthesize(sequence_ids, sample.mel_npy, speaker_id=speaker_id)
        synthesis_sample = SynthesisSample(sample, sample.text, synthesized_audio)
        synthesis_samples.append(synthesis_sample)

    return synthesis_samples


def synthesize(training_samples: List[TrainingSample], path_to_tacotron, path_to_waveglow=None, speaker_id=None):
    model = SynthesisModel(tacotron_path=path_to_tacotron, waveglow_path=path_to_waveglow)

    texts = []
    reference_mels = []

    for sample in training_samples:
        texts.append(sample.text)
        reference_mels.append(sample.mel_npy)

    synthesis_samples = []
    synthesized_audios = model.apply_many_mel(texts, reference_mels, speaker_id)

    for sample, synthesis in zip(training_samples, synthesized_audios):
        synthesis_sample = SynthesisSample(sample, sample.text, synthesis)
        synthesis_samples.append(synthesis_sample)

    return synthesis_samples


def synthesize_every_iter(training_samples, log_dir, start_iter, speaker_id, path_to_save):
    for filename in os.listdir(log_dir):
        suffix = '_snapshot.pth'
        if filename.endswith(suffix):
            iteration = int(filename.replace(suffix, ''))

            path_to_model = os.path.join(log_dir, filename)

            if iteration >= start_iter:
                print(f'ITERATION: {iteration}')
                path_to_synthesized_samples = os.path.join(path_to_save, str(iteration))
                if not os.path.exists(path_to_synthesized_samples):
                    os.makedirs(path_to_synthesized_samples)
                else:
                    print(f'Path {path_to_synthesized_samples} already exists, skipping...')
                    continue

                synthesis_samples = synthesize(training_samples, path_to_tacotron=path_to_model, speaker_id=speaker_id)
                save_synthesized_samples(path_to_synthesized_samples, synthesis_samples)


def create_text_from_chunks_and_reference(text_chunk_sequence, reference_text):
    reference_text = reference_text.split(' ')
    text_chunk_sequence.sort(key=lambda x: x.idx_start)

    cur_chunk_idx = 0
    cur_chunk = text_chunk_sequence[cur_chunk_idx]

    text = []
    for i, word in enumerate(reference_text):
        if i < cur_chunk.idx_start or i > cur_chunk.idx_end:
            text.append(word)
        elif i == cur_chunk.idx_start:
            text.extend(cur_chunk.text_to_synthesize.split(' '))
        elif i == cur_chunk.idx_end:
            text.append(word)
            cur_chunk_idx += 1

            if cur_chunk_idx >= len(text_chunk_sequence):
                continue

            cur_chunk = text_chunk_sequence[cur_chunk_idx]
            if cur_chunk.idx_start <= i:
                raise Exception('Chunks should not intersect')
        else:
            continue

    return text


class ReferencedSynthesizer:
    def __init__(self, model):
        self.model = model

    def produce_many(self, chunks, references):
        texts = []
        reference_audios = []

        print('texts to produce:')
        for chunk_sequence, reference in zip(chunks, references):
            text = create_text_from_chunks_and_reference(chunk_sequence, reference.text)
            text = ' '.join(text)
            print(text)
            texts.append(text)
            reference_audios.append(reference.audio)

        return self.model.apply_many(texts, reference_audios)

    def produce(self, text_chunk_sequence, aligned_reference):
        return self.produce_many([text_chunk_sequence], [aligned_reference])
