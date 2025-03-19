from ..model import TextTransformationStep


class TextTransformer:
    def transform(self, text: str) -> str:
        raise NotImplementedError

    @staticmethod
    def name() -> TextTransformationStep:
        raise NotImplementedError
