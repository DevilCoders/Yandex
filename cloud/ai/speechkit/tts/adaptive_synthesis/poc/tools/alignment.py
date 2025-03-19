import io
import re

from tqdm import tqdm
from yt import wrapper as yt
from ytreader import YTTableParallelReader

from tools.audio import AudioSample
from tools.data_interface.recordings import Recording
from tools.yt_utils import set_yt_config


class WordAlignment:
    def __init__(self, text, phonemes, offset, duration):
        self.text = text
        self.phonemes = phonemes
        self.offset = offset
        self.duration = duration


def space_words_and_punctuation(s):
    s = re.sub(r'([\"\'.,;:!?()]+)', r' \1 ', s)
    s = re.sub(r'\s{2,}', ' ', s)
    return s.strip()


class Alignment:
    def __init__(self, text, json_alignment):
        self.alignment = []
        self.word_index_to_alignment_index = {}
        self.text = text

        reference_words = space_words_and_punctuation(text).split(' ')
        num_words_in_reference_text = len(reference_words)

        word_index = 0
        log = []

        for alignment_index, json_word in enumerate(json_alignment['words']):
            current_word = json_word['text']
            log.append((word_index, current_word))
            phonemes = json_word['phones']
            offset = phonemes[0]['onset']
            offset_last = phonemes[-1]['onset'] + phonemes[-1]['duration']
            duration = offset_last - offset

            self.alignment.append(WordAlignment(current_word, phonemes, offset, duration))

            if word_index >= num_words_in_reference_text:
                assert(current_word == 'yandexttsspecialpauseword')

            if word_index < num_words_in_reference_text and current_word == reference_words[word_index]:
                self.word_index_to_alignment_index[word_index] = alignment_index
                word_index += 1

        log.append(word_index)
        log.append(num_words_in_reference_text)
        log.append(text)

        if word_index != num_words_in_reference_text:
            print('\n'.join([str(value) for value in log]))
            raise Exception("Couldn't parse the alignment")

    def get_word_alignment(self, word_index):
        return self.alignment[self.word_index_to_alignment_index[word_index]]


class AlignedSample(Recording):
    def __init__(self, origin_pool, sample_id, text, audio, alignment):
        super().__init__(origin_pool, sample_id, text, audio)
        self.alignment = alignment


def read_aligned_samples(yt_table):
    set_yt_config(yt.config)
    reader = YTTableParallelReader("hahn", yt_table, cache_size=128, num_readers=1)
    aligned_samples = []

    for line in tqdm(reader):
        wav = line[b'pcm__wav']
        reference_audio = AudioSample(file=io.BytesIO(wav))
        duration = line[b'duration']

        utterance = line[b'utterance']
        reference_text = str(line[b'text'], encoding='utf-8')

        reference_text = re.sub('([,:!?\\.]+)', ' \\1 ', reference_text[:-1])
        reference_text = re.sub('\s+', ' ', reference_text)
        reference_text = reference_text.strip().split(' ')

        sample_id = line[b'ID']
        was_aligned = line[b'error'] is None

        alignment = None
        if was_aligned:
            alignment = Alignment(reference_text, utterance)

        aligned_sample = AlignedSample(yt_table, sample_id, reference_text, reference_audio, alignment)
        aligned_samples.append(aligned_sample)

    return aligned_samples
