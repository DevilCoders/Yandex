import string
import re
from ..model import TextTransformationStep
from .common import TextTransformer


class PunctuationCleaner(TextTransformer):

    def __init__(self):
        self.table = str.maketrans(string.punctuation, " " * len(string.punctuation))

    def transform(self, text: str) -> str:
        # replace punct with whitespace
        text = text.translate(self.table)
        # remove duplicated spaces
        text = re.sub(' +', ' ', text)
        return text.strip()

    @staticmethod
    def name() -> TextTransformationStep:
        return TextTransformationStep.RemovePunctuation
