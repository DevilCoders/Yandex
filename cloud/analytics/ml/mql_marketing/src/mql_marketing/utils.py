import re
import Levenshtein
import pandas as pd
from nltk.util import everygrams
from itertools import product
from collections import Counter, defaultdict
from typing import List


def find_key_words(company_name: str):
    name = (
        company_name
        .replace(' | ', ' || ')
        .replace(' / ', ' || ')
        .replace('\\', ' || ')
        .replace('в прошлом', ' || ')
    )

    name = re.sub(r"[^\w\.\-&\+\'@\|]+", " ", name).strip()
    all_keys = []
    for subname in name.split('||'):
        subname = subname.strip()
        keys = [' '.join(words) for words in everygrams(subname.split(" ")) if ' '.join(words)!='']
        all_keys.extend(keys)

    return list(set(all_keys))


def list_of_all_words(company_name: str):
    name = (
        company_name
        .replace(' | ', ' || ')
        .replace(' / ', ' || ')
        .replace('\\', ' || ')
        .replace('в прошлом', ' || ')
    )

    name = re.sub(r"[^\w\.\-&\+\'@\|]+", " ", name).strip()
    return name.split(' ')


def K_affinity_levenstein(word1: str, word2: str, lower_limit: float = None):
    max_len = max(len(word1), len(word2))
    lev_dist = Levenshtein.distance(word1, word2)
    k_aff_lev = 1.0 - lev_dist/max_len
    if lower_limit is None:
        return k_aff_lev
    return k_aff_lev if k_aff_lev >= lower_limit else 0.0


def K_sorensen_levenstein(token_list_a: List[str], token_list_b: List[str], lower_limit: float = 0.8):
    a, b = len(token_list_a), len(token_list_b)
    c = sum([K_affinity_levenstein(word_a, word_b, lower_limit) for word_a, word_b in product(token_list_a, token_list_b)])
    return 2 * c / (a + b)


class NLP_decomposition:
    word_pattern = re.compile(r"[\w\.\-&\+\'@\|]+")
    nonword_pattern = re.compile(r"[^\w\.\-&\+\'@\|]+")

    def __init__(self, max_df: int = 5, max_df_key: int = 50):
        self.max_df = max_df
        self.max_df_key = max_df_key

        sl = pd.read_csv('stoplist.csv')
        self.stoplist_ = sl['stopwords'].tolist()
        self.stoplist_.append('')
        self.repeats_ = []
        self.all_banned = None
        self.used_keys = defaultdict(int)

    def _meets_reqs(self, phrase: str):
        phrase = re.sub(r"^[\d\.\-&\+\'@\|]+$", "", phrase)
        if (phrase in self.all_banned) or (len(phrase)<=2):
            return False
        if (self.used_keys[phrase]>self.max_df_key):
            return False
        subwords = re.findall(self.word_pattern, phrase)
        if subwords[0] in self.all_banned:
            return False
        if subwords[-1] in self.all_banned:
            return False
        self.used_keys[phrase] += 1
        return True

    @staticmethod
    def transliterate(string: str):
        lower_case_letters = {
            u'а': u'a', u'б': u'b', u'в': u'v', u'г': u'g', u'д': u'd', u'е': u'e', u'ё': u'e',
            u'ж': u'zh', u'з': u'z', u'и': u'i', u'й': u'y', u'к': u'k', u'л': u'l', u'м': u'm',
            u'н': u'n', u'о': u'o', u'п': u'p', u'р': u'r', u'с': u's', u'т': u't', u'у': u'u',
            u'ф': u'f', u'х': u'h', u'ц': u'ts', u'ч': u'ch', u'ш': u'sh', u'щ': u'sch', u'ъ': u'',
            u'ы': u'y', u'ь': u'', u'э': u'e', u'ю': u'yu', u'я': u'ya',
        }

        for cyrillic_string, latin_string in lower_case_letters.items():
            string = string.replace(cyrillic_string, latin_string)
        return string

    def transform(self, df: pd.DataFrame) -> pd.DataFrame:
        def process_delimiters(word):
            return (
                word
                .replace(' | ', ' || ')
                .replace(' / ', ' || ')
                .replace('\\', ' || ')
                .replace('в прошлом', ' || ')
            )

        # all filtered names
        norm_delimiters = [process_delimiters(name) for name in df['name']]
        data_corpus = [re.sub(self.nonword_pattern, " ", name).strip() for name in norm_delimiters]

        # find repeats
        print('Search of repeats...')
        all_keys_total = []
        for name in data_corpus:
            for subname in name.split('||'):
                subname = subname.strip()
                for lang_name in [subname, self.transliterate(subname)]:
                    tokens = [' '.join(words) for words in everygrams(lang_name.split(" "))]
                    all_keys_total.extend(tokens)
        all_keys_total = pd.Series(Counter(all_keys_total)).reset_index()
        all_keys_total.columns = ['keyword', 'cnt']
        all_keys_total['len'] = all_keys_total['keyword'].apply(lambda x: len(x.strip().split(" ")))
        new_stopwords = all_keys_total[(all_keys_total['len']==1) & (all_keys_total['cnt']>100)]['keyword']
        self.stoplist_.extend(new_stopwords)
        new_reps = all_keys_total[(all_keys_total['len']>1) & (all_keys_total['cnt']>self.max_df)]['keyword']
        self.repeats_ = new_reps.tolist()
        self.all_banned = set(self.stoplist_) | set(self.repeats_)

        # generate name-keys
        print('Generate keys...')
        all_keys = []
        for name in data_corpus:
            res_keys = []
            for subname in name.split('||'):
                subname = subname.strip()
                for lang_name in [subname, self.transliterate(subname)]:
                    keys = [' '.join(words) for words in everygrams(lang_name.split(" "))]
                    keys = [key for key in keys if self._meets_reqs(key)]
                    res_keys.extend(keys)
            all_keys.append(list(set(res_keys)))

        df_res = df.copy()
        df_res['raw_keys'] = all_keys
        return df_res
