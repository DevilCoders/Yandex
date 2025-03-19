import pymorphy2
import re
import pyspark.sql.functions as F
import pyspark.sql.types as T
# from nltk.corpus import stopwords
# stop_words = stopwords.words("russian")

ma = pymorphy2.MorphAnalyzer()


def num_digits(s):
    return sum(c.isdigit() for c in s)


def clean_text(text, words_count=100):
    text = text.replace("\\", " ")
    text = text.lower()
    text = re.sub('\-\s\r\n\s{1,}|\-\s\r\n|\r\n', ' ', text)
    text = re.sub(
        '[.,:;_%©?*,!@#$%^&(){{}}]|[+=]|[«»]|[<>]|[\']|[[]|[]]|[/]|"|\s{2,}|-', ' ', text)
    text = ' '.join(word for word in text.split() if len(word) > 2)
    text = ' '.join(word for word in text.split() if not word.isnumeric())
    text = ' '.join(word for word in text.split() if num_digits(word)<=2)
    text = " ".join(ma.parse(word)[0].normal_form for word in text.split())
    # text = " ".join(word for word in text.split() if word not in stop_words)
    text = ' '.join(text.split()[:words_count])
    return text

clean_text_udf = F.udf(clean_text, returnType=T.StringType())


def seq_subarray(phrase, keyword):
    if (phrase is None) or (keyword is None):
        return False
    words = phrase.split()
    keywords = keyword.split()
    if (len(words) == 0) or (len(keywords)==0):
        return False
    for i in range(len(words) - len(keywords) + 1):
        res = None
        for j in range(len(keywords)):
            matched_words = (words[i+j] == keywords[j])
            if res is None:
                res = matched_words
            else:
                res = res and matched_words
        if res:
            return True
    return False

seq_subarray_udf = F.udf(seq_subarray, returnType=T.BooleanType())
