from speechkit.common import Product

from .recognizer import AudioProcessingType, RecognitionConfig, RecognitionModel
from .transcription import Transcription, Word

from .azure import AzureRecognizer
from .yandex import YandexRecognizer


class Singleton(type):
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class RecognizerFabric(metaclass=Singleton):
    products = {}

    def register(self, product: Product, cls: type):
        assert product not in self.products
        self.products[product] = cls

    def create(self, product: Product, **kwargs) -> RecognitionModel:
        return self.products[product](**kwargs)


RecognizerFabric().register(Product.Azure, AzureRecognizer)
RecognizerFabric().register(Product.Yandex, YandexRecognizer)
