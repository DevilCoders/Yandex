from pynorm.fstnormalizer import FSTNormalizer

from ..model import TextTransformationStep
from .common import TextTransformer


class Normalizer(TextTransformer):
    fstnormalizer: FSTNormalizer

    def __init__(self, normalizer_data_path: str, model: str, language_code: str):
        self.fstnormalizer = FSTNormalizer(
            normalizer_data_path='{root}/{model}/{language_code}'.format(
                root=normalizer_data_path, model=model, language_code=language_code
            )
        )

    def transform(self, text: str) -> str:
        result = self.fstnormalizer.normalize(text)
        if isinstance(result, str):
            raise ValueError('failed to normalize text "%s"' % text)
        return result.decode('utf-8')

    @staticmethod
    def name() -> TextTransformationStep:
        return TextTransformationStep.Normalize
