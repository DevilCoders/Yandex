
import regex
import re
from collections import Counter
from numba import njit

def is_lat(s):
    return regex.search(r'\p{IsLatin}', s) is None


def is_cyr(s):
    return regex.search(r'\p{IsCyrillic}', s) is None


def unique_symb(s):
    return len(set(s))

def unique_words(s):
    return len(set(s.split()))



def has_num(inputString):
    return bool(re.search(r'\d', inputString))


def max_freq_char(s):
    res = Counter(s)
    return max(res.values())


def is_company(s):
    s_upper = s.upper()
    return ('ООО' in s_upper) or ('ЗАО' in s_upper) or ('ОАО' in s_upper)


@njit
def n_uppercase(s):
    counter = 0
    for c in s:
        if c.isupper():
            counter += 1
    return counter


def n_words(s):
    return len(s.split())


def short_word_len(s):
    spl = s.split()
    return min([len(w) for w in spl])


def add_names_features(col, features):
    features[col+'_'+'is_cyr'] = features[col].apply(is_cyr)
    features[col+'_'+'n_upper'] = features[col].apply(n_uppercase)
    features[col+'_'+'n_words'] = features[col].apply(n_words)
    features[col+'_'+'is_lat'] = features[col].apply(is_lat)
    features[col+'_'+'name_len'] = features[col].apply(len)
    features[col+'_'+'unique_symb'] = features[col].apply(unique_symb)
    features[col+'_'+'has_num'] = features[col].apply(has_num)
    features[col+'_'+'max_freq_char'] = features[col].apply(max_freq_char)
    features[col+'_'+'is_company'] = features[col].apply(is_company)
    features[col+'_'+'short_word_len'] = features[col].apply(short_word_len)
    features[col+'_'+'unique_words'] = features[col].apply(unique_words)

    return features
