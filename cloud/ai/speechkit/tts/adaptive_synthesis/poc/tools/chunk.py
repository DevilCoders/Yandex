import os
import re
import uuid

from tools.synthesizer import create_text_from_chunks_and_reference


def read_chunk_sequence(chunk_seq_str):
    values = chunk_seq_str.split('*')
    chunk_seq = []
    for i in range(len(values) // 3):
        start_idx, end_idx, text = values[3 * i: 3 * i + 3]
        chunk = Chunk(int(start_idx), int(end_idx), text)
        chunk_seq.append(chunk)
    return chunk_seq


class Chunk:
    def __init__(self, idx_start, idx_end, text_to_synthesize):
        self.idx_start = idx_start
        self.idx_end = idx_end
        self.text_to_synthesize = text_to_synthesize


class ChunkCreator:
    def __init__(self, path_to_dump='tmp_chunks.tsv'):
        self.path_to_dump = path_to_dump
        self.seen_texts = set()
        self.text_to_guid = {}
        self.guid_to_chunks = {}

        if os.path.exists(self.path_to_dump):
            print('found dump loading...')
            self._load_from_dump()

    def _load_from_dump(self):
        with open(self.path_to_dump, 'r') as f:
            f.readline()
            for line in f:
                guid, reference_text, chunk_seq_str = line[:-1].split('\t')
                print(guid, reference_text)
                self.seen_texts.add(reference_text)

                if reference_text not in self.text_to_guid:
                    self.text_to_guid[reference_text] = guid
                    self.guid_to_chunks[guid] = []

                chunk_seq = read_chunk_sequence(chunk_seq_str)
                self.guid_to_chunks[guid].append(chunk_seq)

    def _ask_question(self, question_str, default_answer):
        print(question_str)
        while True:
            answer = input()
            if answer == '':
                return default_answer
            elif answer == 'Y' or answer == 'y' or answer == 'д' or answer == 'Д':
                return True
            elif answer == 'N' or answer == 'n' or answer == 'н' or answer == 'Н':
                return False
            else:
                print('Unknown answer')

    def _dump(self):
        with open(self.path_to_dump, 'w') as f:
            f.write('\t'.join(['guid', 'reference_text', 'chunk_sequence']) + '\n')
            for reference_text, guid in self.text_to_guid.items():
                for chunk_sequence in self.guid_to_chunks[guid]:
                    f.write(guid + '\t' + reference_text + '\t')
                    f.write(
                        '*'.join([
                            str(chunk.idx_start) + '*' + str(chunk.idx_end) + '*' + chunk.text_to_synthesize
                            for chunk in chunk_sequence
                        ]))
                    f.write('\n')

    def create_manually(self, texts):
        for reference_text in texts:
            print('text: ' + ' '.join([f'{i}:{word}' for i, word in enumerate(reference_text.split(' '))]))

            if reference_text in self.seen_texts:
                print('Already seen')
                continue

            guid = str(uuid.uuid4())
            self.text_to_guid[reference_text] = guid

            assert guid not in self.guid_to_chunks
            self.guid_to_chunks[guid] = []

            print(f'guid: {guid}')

            self.seen_texts.add(reference_text)

            stop_collect = False
            while not stop_collect:
                values = re.findall(r'(\d+) (\d+) ([а-яА-ЯёЁ ]+)', input())
                print(values)
                if any([len(parsed_value) != 3 for parsed_value in values]):
                    print('Sorry wrong format')
                    continue

                chunk_seq = []
                for parsed_value in values:
                    start_idx, end_idx, text = parsed_value
                    start_idx = int(start_idx)
                    end_idx = int(end_idx)
                    text = text.rstrip()

                    chunk = Chunk(start_idx, end_idx, text)
                    chunk_seq.append(chunk)

                text_to_synthesize = create_text_from_chunks_and_reference(chunk_seq, reference_text)
                print(f'text_to_synthesize: {text_to_synthesize}')

                if self._ask_question('Is it correct Y/n?', True):
                    self.guid_to_chunks[guid].append(chunk_seq)
                else:
                    print('Bad sample, retrying')

                stop_collect = not self._ask_question('Wanna type another sample? y/N', False)
                print(f'stop_collect={stop_collect}')

            self._dump()

    def get_chunks_for_each_speaker(self, grouped_by_phrases_samples):
        chunks = []
        references = []
        for text, guid in self.text_to_guid.items():
            for speaker, sample in grouped_by_phrases_samples[text].items():
                for chunk_seq in self.guid_to_chunks[guid]:
                    chunks.append(chunk_seq)
                    references.append(sample)

        return chunks, references

    def get_chunks(self, samples):
        text_to_idx = {}

        for i, sample in enumerate(samples):
            text_to_idx[sample.text] = i

        chunks_by_id = {}

        for text, guid in self.text_to_guid.items():
            index = text_to_idx[text]
            chunks_by_id[index] = []

            for chunk_seq in self.guid_to_chunks[guid]:
                chunks_by_id[index].append(chunk_seq)

            print(text)
            print(chunks_by_id[index])

        references = []
        chunks = []

        for i in range(len(samples)):
            for chunk_seq in chunks_by_id[i]:
                references.append(samples[i])
                chunks.append(chunk_seq)

        return chunks, references

