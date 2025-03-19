import random
import re
from typing import Dict, List, Optional

import torch

from acoustic_model.data.text_processor import TextProcessorBase


class DataMask:
    def __call__(self,
                 sequence: List[int],
                 template_text: Optional[str] = None,
                 text_variables: Optional[Dict[str, str]] = None) -> List[int]:
        raise NotImplemented


class RandomWordsMask(DataMask):
    def __init__(self,
                 text_processor: TextProcessorBase,
                 min_coverage: float,
                 max_coverage: float,
                 original_template_prob: float):
        self.text_processor = text_processor
        self.min_coverage = min_coverage
        self.max_coverage = max_coverage
        self.original_template_prob = original_template_prob

    def __call__(self,
                 sequence: List[int],
                 template_text: Optional[str] = None,
                 text_variables: Optional[Dict[str, str]] = None) -> List[int]:
        words = self._get_words(sequence)
        if template_text and text_variables and random.random() < self.original_template_prob:
            # We mask original templates as follows: we take the value corresponding to the variable,
            # break it into words and add their numbers in the source text. And then the old code works,
            # which mask the words from the utterance by their numbers.
            template_text = re.sub(r"[^а-яА-Яa-zA-Z{}_0-9\s]", "", template_text).strip()  # normalize for word splitting
            template_text = re.sub("\s+", " ", template_text)
            template_text_words = template_text.split(" ")
            for var_name, var_value in text_variables.items():
                # normalize for word splitting
                text_variables[var_name] = re.sub(r"[^а-яА-Яa-zA-Z\s]", "", var_value).strip()
                text_variables[var_name] = re.sub("\s+", " ", text_variables[var_name])
                text_variables[var_name] = text_variables[var_name].split(" ")
            text_variables = {f"{{{key}}}": value for key, value in text_variables.items()}
            counter = 0
            expanded_template_words, words_to_mask = [], []
            for template_text_word in template_text_words:
                if template_text_word in text_variables:
                    for var_value_word in text_variables[template_text_word]:
                        expanded_template_words.append(var_value_word)
                        words_to_mask.append(counter)
                        counter += 1
                else:
                    expanded_template_words.append(template_text_word)
                    counter += 1
            assert len(words) == len(expanded_template_words), "'expanded_template_words' doesn't match with utterance"
        else:
            coverage = random.uniform(self.min_coverage, self.max_coverage)
            num_words_to_mask = max(1, round(len(words) * coverage))  # mask at least one word
            words_to_mask = sorted(random.sample(range(len(words)), num_words_to_mask))

        masked_positions = []
        for i, word_idx in enumerate(words_to_mask):
            to_mask = list(words[word_idx])
            # if two words go in a row
            if i + 1 < len(words_to_mask) and words_to_mask[i + 1] - word_idx == 1:
                # mask the gap between them, since there may be a pause that did not enter any of the words
                to_mask += list(range(words[word_idx][-1] + 1, words[word_idx + 1][0]))
            masked_positions.extend(to_mask)

        masked_positions = torch.tensor(masked_positions)
        mask = torch.zeros(len(sequence)).scatter_(0, masked_positions, 1).int().tolist()
        return mask

    def _get_words(self, sequence: List[int]):
        words, current_word = [], []
        # pause_idx = None
        for i, item in enumerate(sequence):
            if not self._is_phone(item):
                # if self._is_pause(item) and words:
                #     assert not current_word and pause_idx is None, "'_get_words' assertion error"
                #     # randomly either add a pause at the end of the current word,
                #     # or save and add to the beginning of the next word
                #     if random.random() < 0.5:
                #         words[-1].extend(range(words[-1][-1] + 1, i + 1))
                #     elif random.random() < 0.5:
                #         pause_idx = i
                if current_word:
                    words.append(current_word)
                    current_word = []
            else:
                # if pause_idx is not None:
                #     assert not current_word, "'_get_words' assertion error"
                #     current_word = list(range(pause_idx, i + 1))
                #     pause_idx = None
                # else:
                current_word.append(i)
        if current_word:
            words.append(current_word)
        return words

    def _is_phone(self, symbol_id):
        return self.text_processor.id_to_symbol[symbol_id] in self.text_processor.phones

    def _is_pause(self, symbol_id):
        return self.text_processor.id_to_symbol[symbol_id].startswith("SIL")


class WordsSequenceMask(RandomWordsMask):
    def __init__(self, text_processor: TextProcessorBase, min_coverage: float, max_coverage: float):
        super().__init__(text_processor, min_coverage, max_coverage, 0.)

    def __call__(self,
                 sequence: List[int],
                 template_text: Optional[str] = None,
                 text_variables: Optional[Dict[str, str]] = None) -> List[int]:
        words = self._get_words(sequence)
        coverage = random.uniform(self.min_coverage, self.max_coverage)
        num_words_to_mask = max(1, round(len(words) * coverage))  # mask at least one word
        shift = random.randint(0, len(words) - num_words_to_mask)
        words_to_mask = words[shift:shift + num_words_to_mask]
        begin_idx, end_idx = words_to_mask[0][0], words_to_mask[-1][-1]
        masked_positions = torch.arange(begin_idx, end_idx + 1)
        mask = torch.zeros(len(sequence)).scatter_(0, masked_positions, 1).int().tolist()
        return mask


class FullMask(DataMask):
    def __init__(self, *args, **kwargs):
        pass

    def __call__(self,
                 sequence: List[int],
                 template_text: Optional[str] = None,
                 text_variables: Optional[Dict[str, str]] = None) -> List[int]:
        mask = torch.ones(len(sequence)).int().tolist()
        return mask


class CompositeMask(DataMask):
    def __init__(self, masks: List[DataMask], weights: List[float]):
        self.masks = masks
        self.weights = weights

    def __call__(self,
                 sequence: List[int],
                 template_text: Optional[str] = None,
                 text_variables: Optional[Dict[str, str]] = None) -> List[int]:
        mask_id = random.choices(range(len(self.masks)), self.weights)[0]
        return self.masks[mask_id](sequence, template_text, text_variables)


def create_mask(text_processor: TextProcessorBase, config: dict):
    mask_types = {
        "random_words_mask": RandomWordsMask,
        "words_sequence_mask": WordsSequenceMask,
        "full_mask": FullMask
    }
    config = config.copy()
    mask_type = config.pop("type")

    if mask_type == "composite_mask":
        masks = [create_mask(text_processor, mask_config) for mask_config in config["masks"]]
        return CompositeMask(masks, config["weights"])

    return mask_types[mask_type](text_processor, **config)
