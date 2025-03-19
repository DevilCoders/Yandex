import json
from .utils import get_text_processor_by_lang


def make_json_sym2id(lang):
    a = get_text_processor_by_lang(lang)

    with open(f'{lang}_sym2id.json', 'w', encoding='utf-8') as f:
        json.dump(a.symbol_to_id, f, ensure_ascii=False, indent=4)
