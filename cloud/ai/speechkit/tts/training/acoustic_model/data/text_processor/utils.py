from .ru import TextProcessorRu
from .de import TextProcessorDe
import json
import os


def get_text_processor_by_lang(lang):
    with open(os.path.dirname(os.path.realpath(__file__)) + f"/../../configs/text_processor/{lang}.json") as f:
        text_processor_config = json.load(f)

    lang2text_processor = {'ru': TextProcessorRu, 'de': TextProcessorDe}

    return lang2text_processor[lang](text_processor_config)
