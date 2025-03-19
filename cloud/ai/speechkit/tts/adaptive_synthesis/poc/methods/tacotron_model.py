import subprocess as sp
import numpy as np
import torch
import json
import tempfile
from typing import List

from contrib.tts.waveglow.glow import WaveGlow

from contrib.tts.tacotron.hparams import create_hparams
from contrib.tts.tacotron.model import Tacotron2, StyleEmbeddingEstimator
from contrib.tts.tacotron.shitova_mel_length import predict_critical_mel_length
from contrib.tts.tacotron.text_processing import utterance_to_symbols, symbols_to_ids, ids_to_symbols, SYMBOLS
from contrib.tts.tacotron.data_utils import TextMelDataset, TextMelCollate, SPEAKERS
from contrib.tts.pipeline.extract_mel import mel_spectrogram

from tools.audio import AudioSample
from tools.data import sample_sox, normalize_volume

import sys
import os

os.environ["CUDA_DEVICE_ORDER"] = "PCI_BUS_ID"
os.environ["CUDA_VISIBLE_DEVICES"] = "1"


def load_tacotron(checkpoint_path):
    ckpt = torch.load(checkpoint_path)
    hparams = create_hparams(ckpt['params'])

    ckpt['state_dict'] = {
        key.replace('module.', ''): value
        for key, value in ckpt['state_dict'].items()
    }

    tacotron = Tacotron2(hparams).cuda().eval()
    tacotron.load_state_dict(ckpt['state_dict'])
    return tacotron, hparams


def load_waveglow(path):
    ckpt = torch.load(path)
    waveglow = WaveGlow(**ckpt['config'])

    def remove(conv_list):
        new_conv_list = torch.nn.ModuleList()
        for old_conv in conv_list:
            old_conv = torch.nn.utils.remove_weight_norm(old_conv)
            new_conv_list.append(old_conv)
        return new_conv_list

    for wn in waveglow.WN:
        wn.start = torch.nn.utils.remove_weight_norm(wn.start)
        wn.in_layers = remove(wn.in_layers)
        wn.cond_layers = remove(wn.cond_layers)
        wn.res_skip_layers = remove(wn.res_skip_layers)

    waveglow.load_state_dict(ckpt['state_dict'])
    return waveglow.cuda().eval()


def load_sequences(txt_path, add_bos_eos=False):
    sequences = []
    voice_texts = []
    sample_ids = []

    with open(txt_path) as f:
        for sample_id, line in enumerate(f):
            utterances = []
            text = ''
            utt_json = json.loads(line)
            for i, utt in enumerate(utt_json):
                text = text + utt['text']
                assert 'phonemes' in utt

                utt = ['@' + s if '@' + s in SYMBOLS else s for s in utt['phonemes']]
                utt = ['.' if t == '...' else t for t in utt]
                utt = [t for t in utt if t in SYMBOLS]
                assert tuple(ids_to_symbols(symbols_to_ids(utt))) == tuple(utt)

                if i < len(utt_json) - 1:
                    utt = utt + ['SIL3']

                utterances.extend(utt)

            assert add_bos_eos
            if add_bos_eos:
                utterances = ["@BOS"] + utterances + ["@EOS"]

            sample_ids.append(sample_id)
            sequences.append(symbols_to_ids(utterances))
            voice_texts.append(text)

    return sample_ids, sequences, voice_texts


def calc_mel_spectrogram(audio):
    return mel_spectrogram(np.asfortranarray(audio.data))


class TacotronPreprocessor:
    # cd arcadia && ./ya make -j16 ./voicetech/tts/tt_preprocessor_cli/tt_preprocessor_cli
    PREPROCESSOR_CLI = '~/arcadia/voicetech/tts/tt_preprocessor_cli/tt_preprocessor_cli'
    LINGWARE = '../models/tacotron_resource'  # sbr:1306660626
    CACHE_DIR = '../models/tacotron_preprocessor/'
    CACHE_PATH = f'{CACHE_DIR}cache'

    def __init__(self, lingware_path=None, preprocessor_cli_path=None):
        self.cache = {}
        self.lingware_path = TacotronPreprocessor.LINGWARE if lingware_path is None else lingware_path
        self.preprocessor_cli_path = preprocessor_cli_path
        if preprocessor_cli_path is None:
            self.preprocessor_cli_path = TacotronPreprocessor.PREPROCESSOR_CLI

        if os.path.exists(TacotronPreprocessor.CACHE_PATH):
            with open(TacotronPreprocessor.CACHE_PATH, 'r') as f:
                self.cache = json.load(f)
        elif not os.path.exists(TacotronPreprocessor.CACHE_DIR):
            os.makedirs(TacotronPreprocessor.CACHE_DIR)

    def update_cache(self, new_texts, new_sequences):
        for text, sequence in zip(new_texts, new_sequences):
            self.cache[text] = sequence
        with open(TacotronPreprocessor.CACHE_PATH, 'w') as f:
            json.dump(self.cache, f, indent=4, sort_keys=True)

    def _tt_preprocess_cli(self, texts, add_bos_eos=False):
        with tempfile.TemporaryDirectory() as tmp:
            file_with_texts = os.path.join(tmp, 'texts.txt')
            file_with_sequences = os.path.join(tmp, 'sequences.txt')

            print(f'saving texts to {file_with_texts}')
            with open(file_with_texts, 'w') as f:
                for text in texts:
                    f.write(text + '\n')

            print('converting texts to phoneme sequences')
            # convert texts to phoneme sequences

            sp.check_call(
                f"{self.preprocessor_cli_path} " +
                f"--lingware {self.lingware_path} " +
                f"--input {file_with_texts} " +
                f"--json-output > {file_with_sequences}",
                shell=True
            )

            print(f'load sequences from {file_with_sequences}')
            sample_ids, new_sequences, _ = load_sequences(file_with_sequences, add_bos_eos)
            assert sample_ids == list(range(len(texts)))

            self.update_cache(texts, new_sequences)
            return new_sequences

    def check_all_in_cache(self, texts):
        new_texts = []
        for text in texts:
            if text not in self.cache:
                new_texts.append(text)
        return new_texts

    def process(self, texts, add_bos_eos=False):
        assert add_bos_eos

        new_texts = self.check_all_in_cache(texts)
        if len(new_texts) != 0:
            self._tt_preprocess_cli(new_texts, add_bos_eos)

        sequences = [self.cache[text] for text in texts]
        return sequences


def prepare_audio_format(audios: List[AudioSample]):
    audios = sample_sox(audios, SynthesisModel.MODEL_SAMPLE_RATE)
    audios = normalize_volume(audios)
    return audios


class SynthesisModel:
    MODEL_SAMPLE_RATE = 22050

    SKIP_EXTRA_SPACES = False  # --skip-extra-spaces
    INSERT_BOS_EOS = True  # --insert-bos-eos

    WAVEGLOW_CHECKPOINT = '../models/multi_ft.pth'  # sbr:1311731560
    TACOTRON_CHECKPOINT = '../models/model_without_speaker_id/0128000_snapshot.pth'  # sbr:1311732371

    def __init__(self, waveglow_path=None, tacotron_path=None, lingware_path=None, preprocessor_cli_path=None):
        self.waveglow_path = SynthesisModel.WAVEGLOW_CHECKPOINT if waveglow_path is None else waveglow_path
        self.tacotron_path = SynthesisModel.TACOTRON_CHECKPOINT if tacotron_path is None else tacotron_path
        self.waveglow = load_waveglow(self.waveglow_path)
        self.tacotron, self.hparams = load_tacotron(self.tacotron_path)
        self.tacotron_preprocessor = TacotronPreprocessor(lingware_path, preprocessor_cli_path)

        if self.tacotron_path == SynthesisModel.TACOTRON_CHECKPOINT:
            assert self.hparams.add_spaces == (not SynthesisModel.SKIP_EXTRA_SPACES)
            assert self.hparams.bos_eos == SynthesisModel.INSERT_BOS_EOS

    def gst(self, mel):
        with torch.no_grad():
            length = torch.LongTensor([mel.size(1)])
            x, _ = self.tacotron.gst(mel.cuda().unsqueeze(0), length.cuda())
            return x.squeeze(0).cpu().numpy()

    def synthesize_mel(self, seq, ref_mel, speaker_id=None):
        if self.hparams.speaker_embedding:
            assert speaker_id is not None
        else:
            speaker_id = 'alexr_male'

        gst_mels = torch.from_numpy(ref_mel).unsqueeze(0)
        gst_mel_lengths = torch.LongTensor([gst_mels.size(-1)])

        crit_length = predict_critical_mel_length(seq)
        texts, text_lengths = torch.LongTensor(seq).unsqueeze(0), torch.LongTensor([len(seq)])

        with torch.no_grad():
            # set maximum allowed number of decoder steps
            self.tacotron.decoder.max_decoder_steps = crit_length + 100

            # convert speaker name to id
            speaker_ids = torch.LongTensor([SPEAKERS[speaker_id]]).cuda()

            # tacotron inference
            _, mels, _, alignments, _, _, _ = self.tacotron.forward(
                texts.cuda(), text_lengths.cuda(),
                None, None, speaker_ids,
                gst_mels=gst_mels.cuda(),
                gst_mel_lengths=gst_mel_lengths.cuda())

        return mels[0]

    def vocode(self, mel):
        if not isinstance(mel, torch.Tensor):
            mel = torch.from_numpy(mel)

        with torch.no_grad():
            audio = self.waveglow.infer(mel.cuda().unsqueeze(0))[0].data.cpu().numpy()
        return audio

    def synthesize(self, sequence, reference_mel, speaker_id=None):
        print(" ".join([s.replace('@', '') for s in ids_to_symbols(sequence)]))
        mel = self.synthesize_mel(sequence, reference_mel, speaker_id)
        audio = AudioSample(data=self.vocode(mel), sample_rate=SynthesisModel.MODEL_SAMPLE_RATE)
        return audio

    def apply_many_mel(self, texts, reference_mels, speaker_id=None):
        sequences = self.tacotron_preprocessor.process(texts, add_bos_eos=SynthesisModel.INSERT_BOS_EOS)

        audios = []
        for sample_id, sequence in enumerate(sequences):
            print(f'sample_idx: {sample_id}')
            reference_mel = reference_mels[sample_id]
            audio = self.synthesize(sequence, reference_mel, speaker_id)
            audios.append(audio)

        audios = normalize_volume(audios)

        return audios

    def apply_many(self, texts, reference_audios, speaker_id=None):
        reference_audios = prepare_audio_format(reference_audios)
        reference_mels = []
        for reference_audio in reference_audios:
            reference_mel = calc_mel_spectrogram(reference_audio)
            reference_mels.append(reference_mel)

        return self.apply_many_mel(texts, reference_mels, speaker_id)

    def apply(self, text, reference_audio, speaker_id=None):
        return self.apply_many([text], [reference_audio], speaker_id)[0]
