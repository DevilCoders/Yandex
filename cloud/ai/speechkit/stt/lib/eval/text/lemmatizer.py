from pymorphy2 import MorphAnalyzer

from ..model import TextTransformationStep
from .common import TextTransformer


class Lemmatizer(TextTransformer):
    morphy: MorphAnalyzer

    def __init__(self):
        self.morphy = MorphAnalyzer()

    def transform(self, text: str) -> str:
        tokens = text.split(' ')
        return ' '.join([self.morphy.parse(token)[0].normal_form for token in tokens])

    @staticmethod
    def name() -> TextTransformationStep:
        return TextTransformationStep.Lemmatize
