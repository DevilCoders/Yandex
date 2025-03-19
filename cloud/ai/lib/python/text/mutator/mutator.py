import typing

from cloud.ai.lib.python.log import get_logger

from enum import Enum
from collections import defaultdict
import pylev
import pymorphy2
import random
from abc import ABC, abstractmethod
from dataclasses import dataclass

logger = get_logger(__name__)


class MutationType(Enum):
    DROP_WORD = 'drop_word'
    SWAP_WORDS = 'swap_words'
    ADD_NOT = 'add_not'
    LEMMATIZE = 'lemmatize'
    SWAP_ADJACENT_LETTERS = 'swap_adjacent_letters'
    MISTYPE = 'mistype'
    MISTAKE = 'mistake'
    CLOSE_LEV_WORD = 'close_lev_word'
    ADD_QUESTION = 'add_question'


@dataclass
class TextWrapper:
    words: typing.List[str]
    mask: typing.List[bool]


@dataclass
class MutationResult:
    applied: bool
    text: TextWrapper


class Mutation(ABC):
    parser = pymorphy2.MorphAnalyzer()

    @staticmethod
    @abstractmethod
    def name() -> MutationType:
        pass

    @staticmethod
    @abstractmethod
    def weight() -> int:
        pass

    @abstractmethod
    def mutate(self, text: TextWrapper) -> MutationResult:
        pass

    @staticmethod
    def random_index(mask: typing.List[bool]) -> int:
        return random.choice([idx for idx, elem in enumerate(mask) if elem])

    @staticmethod
    def random_indices(mask: typing.List[bool], count=2) -> typing.List[int]:
        return random.sample([idx for idx, elem in enumerate(mask) if elem], count)

    @staticmethod
    def lemmatize(word: str):
        return Mutation.parser.parse(word)[0].normal_form

    @staticmethod
    def change_word(new_word: str, index: int, text: TextWrapper) -> TextWrapper:
        words, mask = text.words, text.mask
        words[index] = new_word
        mask[index] = False
        return TextWrapper(words=words, mask=mask)

    @staticmethod
    def try_change_word(new_word: str, index: int, text: TextWrapper) -> MutationResult:
        if text.words[index] == new_word:
            return MutationResult(applied=False, text=text)

        return MutationResult(applied=True, text=Mutation.change_word(new_word, index, text))

    @staticmethod
    def needed_words():
        return 1

    def check_mask(self, mask: typing.List[bool]) -> bool:
        return sum(mask) >= self.needed_words()


class DropWord(Mutation):
    @staticmethod
    def needed_words():
        return 2    # we need something else to be left after deletion

    @staticmethod
    def name() -> MutationType:
        return MutationType.DROP_WORD

    @staticmethod
    def weight() -> int:
        return 9

    def mutate(self, text: TextWrapper) -> MutationResult:
        chosen_idx = self.random_index(text.mask)
        return MutationResult(
            applied=True,
            text=TextWrapper(
                words=text.words[:chosen_idx] + text.words[chosen_idx + 1:],
                mask=text.mask[:chosen_idx] + text.mask[chosen_idx + 1:],
            ),
        )


class SwapWords(Mutation):

    @staticmethod
    def needed_words():
        return 2

    @staticmethod
    def name() -> MutationType:
        return MutationType.SWAP_WORDS

    @staticmethod
    def weight() -> int:
        return 5

    def mutate(self, text: TextWrapper) -> MutationResult:
        i, j = self.random_indices(text.mask, 2)
        if i == j or text.words[i] == text.words[j]:
            return MutationResult(applied=False, text=text)
        words, mask = text.words, text.mask
        words[i], words[j] = words[j], words[i]
        mask[i] = False
        mask[j] = False
        return MutationResult(applied=True, text=TextWrapper(words=words, mask=mask))


class AddNot(Mutation):

    def is_verb(self, word):
        return self.parser.parse(word)[0].tag.POS == 'VERB'

    @staticmethod
    def name() -> MutationType:
        return MutationType.ADD_NOT

    @staticmethod
    def weight() -> int:
        return 6

    def mutate(self, text: TextWrapper) -> MutationResult:
        to_insert = []
        for (idx, word), m in zip(enumerate(text.words), text.mask):
            if m and self.is_verb(word) and (idx == 0 or text.words[idx - 1] != 'не'):
                to_insert.append(idx)

        if not to_insert:
            return MutationResult(applied=False, text=text)
        chosen_idx = random.choice(to_insert)

        words = text.words[:chosen_idx] + ['не'] + text.words[chosen_idx:]
        mask = text.mask[:chosen_idx] + [False] + text.mask[chosen_idx:]
        return MutationResult(applied=True, text=TextWrapper(words=words, mask=mask))


class Lemmatize(Mutation):

    @staticmethod
    def name() -> MutationType:
        return MutationType.LEMMATIZE

    @staticmethod
    def weight() -> int:
        return 3

    def mutate(self, text: TextWrapper) -> MutationResult:
        chosen_idx = self.random_index(text.mask)
        lemmatized_word = self.lemmatize(text.words[chosen_idx])
        return self.try_change_word(lemmatized_word, chosen_idx, text)


class SwapAdjacentLetters(Mutation):

    def __init__(self, min_word_length=4):
        self.min_word_length = min_word_length

    @staticmethod
    def name() -> MutationType:
        return MutationType.SWAP_ADJACENT_LETTERS

    @staticmethod
    def weight() -> int:
        return 2

    def mutate(self, text: TextWrapper) -> MutationResult:
        chosen_idx = self.random_index(text.mask)
        word = text.words[chosen_idx]

        if len(word) < self.min_word_length:
            return MutationResult(applied=False, text=text)

        idx = random.randint(0, len(word) - 2)
        letters = list(word)
        letters[idx], letters[idx + 1] = letters[idx + 1], letters[idx]
        new_word = ''.join(letters)
        return self.try_change_word(new_word, chosen_idx, text)


class Mistype(Mutation):

    def __init__(self, lang='ru-RU', min_word_length=4):
        self.min_word_length = min_word_length
        self.lang = lang
        russian_default_keyboard = [
            list("йцукенгшщзхъ"),
            list(" фывапролджэ"),
            list("  ячсмитьбю "),
        ]
        kazakh_modified_keyboard = [
            list("йңукенгшғзһұ"),
            list(" өықапролджә"),
            list("  яісмитүбю "),
        ]
        kazakh_default_keyboard = [
            list(" әіңғ  үұқөһ"),
            list("йцукенгшщзхъ"),
            list(" фывапролджэ"),
            list("  ячсмитьбю "),
        ]

        self.lang_to_keyboards = {
            'ru-RU': [
                russian_default_keyboard,
            ],
            'kk-KK': [
                russian_default_keyboard,
                kazakh_modified_keyboard,
                kazakh_default_keyboard,
            ],
        }

        additional_mappings = {
            'е': 'ё',
            'ң': 'ц',
            'ғ': 'щ',
            'һ': 'х',
            'ұ': 'ъ',
            'ө': 'ф',
            'қ': 'в',
            'ә': 'э',
            'і': 'ч',
            'ү': 'ь',
        }
        self.mappings = []
        for keyboard in self.lang_to_keyboards[self.lang]:

            mapping = {}
            for i in range(len(keyboard)):
                for j in range(len(keyboard[i])):
                    if keyboard[i][j] != ' ':
                        mapping[keyboard[i][j]] = (i, j)

            for letter in additional_mappings:
                if letter in mapping:
                    mapping[additional_mappings[letter]] = mapping[letter]
            self.mappings.append(mapping)

    @staticmethod
    def name() -> MutationType:
        return MutationType.MISTYPE

    @staticmethod
    def weight() -> int:
        return 2

    def mutate(self, text: TextWrapper) -> MutationResult:
        chosen_idx = self.random_index(text.mask)
        word = text.words[chosen_idx]

        if len(word) < self.min_word_length:
            return MutationResult(applied=False, text=text)

        idx = random.choice(list(range(len(word))))
        letter = word[idx]

        for mapping in self.mappings:
            if letter not in mapping:
                continue
            cur_i, cur_j = mapping[letter]
            to_change = []
            for c, (i, j) in mapping.items():
                if c != letter and -1 <= cur_i - i <= 1 and -1 <= cur_j - j <= 1:
                    to_change.append(c)

            letters = list(word)
            letters[idx] = random.choice(to_change)
            new_word = ''.join(letters)
            return self.try_change_word(new_word, chosen_idx, text)

        return MutationResult(applied=False, text=text)


class Mistake(Mutation):

    def __init__(self, min_word_length=4):
        self.min_word_length = min_word_length
        self.mistake_mappings = {
            'ь': ['', 'ъ'],
            'ъ': ['', 'ь'],
            'о': ['а'],
            'а': ['о'],
            'е': ['и'],
            'и': ['е'],
            'р': ['л'],
            'л': ['р'],
            'ё': ['о'],
        }

    @staticmethod
    def name() -> MutationType:
        return MutationType.MISTAKE

    @staticmethod
    def weight() -> int:
        return 2

    def mutate(self, text: TextWrapper) -> MutationResult:
        chosen_idx = self.random_index(text.mask)
        word = text.words[chosen_idx]

        if len(word) < self.min_word_length:
            return MutationResult(applied=False, text=text)

        to_change = [idx for idx, c in enumerate(word) if c in self.mistake_mappings]
        if not to_change:
            return MutationResult(applied=False, text=text)

        idx = random.choice(to_change)
        letters = list(word)
        letters[idx] = random.choice(self.mistake_mappings[word[idx]])
        new_word = ''.join(letters)
        return self.try_change_word(new_word, chosen_idx, text)


class CloseLevWord(Mutation):

    def __init__(self, vocab: typing.Dict[str, typing.List[str]], min_lev_acceptable_dist: int = 0,
                 extra_acceptable_length: int = 1):
        self.vocab = vocab
        self.min_lev_acceptable_dist = min_lev_acceptable_dist
        self.extra_acceptable_length = extra_acceptable_length

    @staticmethod
    def name() -> MutationType:
        return MutationType.CLOSE_LEV_WORD

    @staticmethod
    def weight() -> int:
        return 5

    def find_nearest_levenshtein(self, word: str):
        # todo: maybe normalize over len(word)
        min_dist = float('inf')
        best_words = []

        if word in self.vocab:
            words = self.vocab[word]
        else:
            words = self.vocab.keys()
            logger.debug(f'Word "{word}" not found in precomputed dict, defaulting to full search')

        for other in words:
            if other != word and self.lemmatize(word) != self.lemmatize(other):
                d = pylev.levenshtein(word, other)
                if self.min_lev_acceptable_dist <= d < min_dist:
                    min_dist = d
                    best_words = [(other_word, other_distance)
                                  for other_word, other_distance in best_words
                                  if other_distance <= min_dist + self.extra_acceptable_length] + [(other, d)]
                elif d <= min_dist + self.extra_acceptable_length:
                    best_words.append((other, d))

        return [other_word for other_word, _ in best_words], min_dist

    def mutate(self, text: TextWrapper) -> MutationResult:
        chosen_idx = self.random_index(text.mask)
        word = text.words[chosen_idx]
        best_words, distance = self.find_nearest_levenshtein(word)
        if not best_words:
            return MutationResult(applied=False, text=text)
        new_word = random.choice(best_words)
        return self.try_change_word(new_word, chosen_idx, text)


class AddQuestion(Mutation):

    def __init__(self, min_words_in_row_dropped: int = 2, max_words_in_row_dropped: int = 4, min_word_length: int = 5):
        self.min_words_in_row_dropped = min_words_in_row_dropped
        self.max_words_in_row_dropped = max_words_in_row_dropped
        self.min_word_length = min_word_length
        self.min_words_in_row_dropped = min_words_in_row_dropped

    def needed_words(self):
        return self.min_words_in_row_dropped

    @staticmethod
    def name() -> MutationType:
        return MutationType.ADD_QUESTION

    @staticmethod
    def weight() -> int:
        return 6

    def mutate(self, text: TextWrapper) -> MutationResult:
        possible_choices = []

        for start in range(len(text.words)):
            if not text.mask[start]:
                continue
            found_words = 0 if len(text.words[start]) < self.min_word_length else 1
            for end in range(start, len(text.words)):
                if not text.mask[end]:
                    break
                found_words += 0 if (start == end or len(text.words[end]) < self.min_word_length) else 1
                if found_words > self.max_words_in_row_dropped:
                    break
                if found_words >= self.min_words_in_row_dropped:
                    possible_choices.append((start, end))

        if not possible_choices:
            return MutationResult(applied=False, text=text)

        chosen_start, chosen_end = random.choice(possible_choices)

        words = text.words[:chosen_start] + ['?'] + text.words[chosen_end + 1:]
        mask = text.mask[:chosen_start] + [False] + text.mask[chosen_end + 1:]
        return MutationResult(applied=True, text=TextWrapper(words=words, mask=mask))


@dataclass
class MutatedText:
    old_text: str
    new_text: str
    changed: bool
    applied_mutations: typing.List[MutationType]


@dataclass
class AppliedMutation:
    can_continue: bool
    text: TextWrapper
    mutation: typing.Optional[Mutation]


class Mutator:
    def __init__(
        self,
        lang: str = 'ru-RU',
        weight_limit: int = 10,
        prior_mutate_probability: float = 1.0,
        mutations_whitelist: typing.Optional[typing.List[MutationType]] = None,
        max_words_mutated_percentage: float = 0.5,
        mutations_kwargs_overrides: typing.Optional[typing.Dict[MutationType, typing.Dict[str, typing.Any]]] = None,
    ):
        assert 0.0 <= prior_mutate_probability <= 1.0
        mutations_kwargs_overrides = defaultdict(dict, mutations_kwargs_overrides or {})

        available_mutation_classes = {
            'ru-RU': [
                DropWord,
                SwapWords,
                AddNot,
                Lemmatize,
                SwapAdjacentLetters,
                Mistype,
                Mistake,
                AddQuestion,
            ],
            'kk-KK': [
                DropWord,
                SwapWords,
                SwapAdjacentLetters,
                Mistype,
                Mistake,
                AddQuestion,
            ],
        }[lang]

        mutations_kwargs_overrides[MutationType.MISTYPE]['lang'] = mutations_kwargs_overrides[MutationType.MISTYPE].get(
            'lang', lang
        )

        if mutations_kwargs_overrides[MutationType.CLOSE_LEV_WORD]:
            assert 'vocab' in mutations_kwargs_overrides[MutationType.CLOSE_LEV_WORD], 'you have to provide vocab'
            available_mutation_classes.append(CloseLevWord)

        if mutations_whitelist is not None:
            available_mutation_classes = [
                mutation_class
                for mutation_class in available_mutation_classes
                if mutation_class.name() in mutations_whitelist
            ]

        available_mutations = []

        for mutation_class in available_mutation_classes:
            mutation_name = mutation_class.name()
            kwargs = mutations_kwargs_overrides.get(mutation_name, {})
            available_mutations.append(mutation_class(**kwargs))

        self.weight_limit: int = weight_limit
        self.mutations = available_mutations
        self.prior_mutate_probability = prior_mutate_probability
        self.max_words_mutated_percentage = max_words_mutated_percentage

    @staticmethod
    def get_text_wrapper(text: str) -> TextWrapper:
        words = text.split()
        return TextWrapper(words=words, mask=[True] * len(words))

    @staticmethod
    def gather(words: typing.List[str]):
        return ' '.join(words)

    def mutate(self, text: str) -> MutatedText:
        assert text == text.lower()

        if random.random() >= self.prior_mutate_probability:
            return MutatedText(old_text=text, new_text=text, changed=False, applied_mutations=[])

        weight = 0
        can_continue = True
        text_wrapper = self.get_text_wrapper(text)
        iteration_count = 0
        applied_mutations = []
        while can_continue and iteration_count < len(text_wrapper.words) * self.max_words_mutated_percentage:
            applied_result = self.add_mutation(text_wrapper, weight)
            can_continue, text_wrapper = applied_result.can_continue, applied_result.text
            if applied_result.mutation is not None:
                applied_mutations.append(applied_result.mutation.name())
                weight += applied_result.mutation.weight()
            iteration_count += 1

        new_text = self.gather(text_wrapper.words)
        return MutatedText(
            old_text=text,
            new_text=new_text,
            changed=text != new_text,
            applied_mutations=applied_mutations,
        )

    def add_mutation(self, text: TextWrapper, current_weight: int) -> AppliedMutation:
        if current_weight >= self.weight_limit:
            return AppliedMutation(can_continue=False, text=text, mutation=None)

        chosen_mutation: Mutation = random.choice(self.mutations)
        chosen_weight = chosen_mutation.weight()

        if current_weight + chosen_weight > self.weight_limit:
            # todo we can change can_continue to True here if we want to continue applying smaller mutations
            return AppliedMutation(can_continue=False, text=text, mutation=None)
        if not chosen_mutation.check_mask(text.mask):
            return AppliedMutation(can_continue=False, text=text, mutation=None)

        mutation_result = chosen_mutation.mutate(text)

        return AppliedMutation(
            can_continue=True, text=mutation_result.text, mutation=chosen_mutation if mutation_result.applied else None
        )
