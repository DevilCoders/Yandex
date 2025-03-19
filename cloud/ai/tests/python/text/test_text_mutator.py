import unittest
from cloud.ai.lib.python.text.mutator import (
    Mutator, AddNot, DropWord, Lemmatize, SwapWords, SwapAdjacentLetters, Mistype, Mistake, CloseLevWord, MutationType,
    AddQuestion,
)
from mock import patch


class TestMutation(unittest.TestCase):
    def test_drop_word(self):
        mt = DropWord()
        mutation_result = mt.mutate(Mutator.get_text_wrapper('в этом предложении пять слов'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(4, len(mutation_result.text.words))
        self.assertEqual(4, len(mutation_result.text.mask))

    def test_swap_words(self):
        mt = SwapWords()
        mutation_result = mt.mutate(Mutator.get_text_wrapper('раз два'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(['два', 'раз'], mutation_result.text.words)
        self.assertEqual([False] * 2, mutation_result.text.mask)

    def test_add_not(self):
        mt = AddNot()
        mutation_result = mt.mutate(Mutator.get_text_wrapper('да приходите к нам сегодня на ужин'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual('да не приходите к нам сегодня на ужин'.split(), mutation_result.text.words)
        self.assertEqual([True, False] + [True] * 6, mutation_result.text.mask)

    def test_lemmatize(self):
        mt = Lemmatize()
        mutation_result = mt.mutate(Mutator.get_text_wrapper('приходите'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(['приходить'], mutation_result.text.words)
        self.assertEqual([False], mutation_result.text.mask)

    def test_swap_adjacent_letters(self):
        mt = SwapAdjacentLetters()
        mutation_result = mt.mutate(Mutator.get_text_wrapper('вода'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(1, len(mutation_result.text.words))
        self.assertIn(mutation_result.text.words[0], ['овда', 'вдоа', 'воад'])
        self.assertEqual([False], mutation_result.text.mask)

    def test_mistype(self):
        mt = Mistype(min_word_length=1)
        mutation_result = mt.mutate(Mutator.get_text_wrapper('р'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(1, len(mutation_result.text.words))
        self.assertIn(mutation_result.text.words[0], 'нгопитшм')
        self.assertEqual([False], mutation_result.text.mask)

    def test_mistake(self):
        mt = Mistake(min_word_length=1)
        mutation_result = mt.mutate(Mutator.get_text_wrapper('порт'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(1, len(mutation_result.text.words))
        self.assertIn(mutation_result.text.words[0], ['парт', 'полт'])
        self.assertEqual([False], mutation_result.text.mask)

    def test_close_lev_word(self):
        mt = CloseLevWord(vocab={'петербург': ['петергоф', 'питер', 'спб', 'екатеринбург']}, extra_acceptable_length=0)
        mutation_result = mt.mutate(Mutator.get_text_wrapper('петербург'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(['петергоф'], mutation_result.text.words)
        self.assertEqual([False], mutation_result.text.mask)

    def test_close_lev_word_unknown(self):
        mt = CloseLevWord(vocab={
            'петербург': ['петергоф', 'питер', 'спб', 'екатеринбург'],
            'караван': ['кара', 'кран'],
        }, extra_acceptable_length=0)
        mutation_result = mt.mutate(Mutator.get_text_wrapper('корова'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(['караван'], mutation_result.text.words)
        self.assertEqual([False], mutation_result.text.mask)

    def test_add_question(self):
        mt = AddQuestion(max_words_in_row_dropped=1, min_words_in_row_dropped=1, min_word_length=1)
        mutation_result = mt.mutate(Mutator.get_text_wrapper('текст'))

        self.assertTrue(mutation_result.applied)
        self.assertEqual(1, len(mutation_result.text.words))
        self.assertIn(mutation_result.text.words[0], '?')
        self.assertEqual([False], mutation_result.text.mask)

    @patch('random.choice')
    def test_add_question_with_choice(self, choice):
        text = 'один два три четыре пять'
        words = text.split()
        for start in range(len(words)):
            choice.return_value = (start, start)

            mt = AddQuestion(max_words_in_row_dropped=1, min_words_in_row_dropped=1, min_word_length=1)
            mutation_result = mt.mutate(Mutator.get_text_wrapper(text))
            new_words = words.copy()
            new_words[start] = '?'
            self.assertEqual(True, mutation_result.applied)
            self.assertEqual(new_words, mutation_result.text.words)


class TestMutator(unittest.TestCase):
    def test_mutator_default_constructor(self):
        mt = Mutator()
        old_text = 'текст из нескольких слов'
        mutated_text = mt.mutate(old_text)
        self.assertEqual(old_text, mutated_text.old_text)

        if mutated_text.changed:
            self.assertNotEqual(old_text, mutated_text.new_text)
            self.assertLess(0, len(mutated_text.applied_mutations))
        else:
            self.assertEqual(old_text, mutated_text.new_text)
            self.assertEqual(0, len(mutated_text.applied_mutations))

    def test_mutator_no_vocab(self):
        self.assertRaises(Exception, Mutator,
                          mutations_whitelist=[MutationType.CLOSE_LEV_WORD],
                          mutations_kwargs_overrides={MutationType.CLOSE_LEV_WORD: {'extra_acceptable_length': 0}})

    def test_mutations_whitelist(self):
        mt = Mutator(
            mutations_whitelist=[MutationType.CLOSE_LEV_WORD],
            mutations_kwargs_overrides={MutationType.CLOSE_LEV_WORD: {
                'extra_acceptable_length': 0,
                'vocab': {'петербург': ['петергоф', 'питер', 'спб', 'екатеринбург']},
            }},
        )
        old_text = 'петербург'
        mutated_text = mt.mutate(old_text)

        self.assertEqual(old_text, mutated_text.old_text)
        self.assertEqual(mutated_text.new_text, 'петергоф')
        self.assertEqual([MutationType.CLOSE_LEV_WORD], mutated_text.applied_mutations)

    def test_mutator_kazakh(self):
        mt = Mutator(lang='kk-KK')
        old_text = 'он сегіз жастан бастап қарастырамыз сіздің жасыңыз қанша'
        mutated_text = mt.mutate(old_text)
        self.assertTrue(mutated_text.changed)
        self.assertNotEqual(old_text, mutated_text.new_text)
