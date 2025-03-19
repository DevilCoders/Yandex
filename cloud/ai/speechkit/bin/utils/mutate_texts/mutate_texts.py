import json

import nirvana.job_context as nv
import cloud.ai.lib.python.text.mutator as mutator


def main():
    op_ctx = nv.context()

    with open(op_ctx.inputs.get('texts.json')) as f:
        texts = json.load(f)

    if op_ctx.inputs.has('levenshtein_dict.json'):
        with open(op_ctx.inputs.get('levenshtein_dict.json')) as f:
            levenshtein_dict = json.load(f)
    else:
        levenshtein_dict = None

    lang = op_ctx.parameters.get('lang')
    weight_limit = op_ctx.parameters.get('weight-limit')
    whitelist = [mutator.MutationType(name) for name in op_ctx.parameters.get('whitelist-mutations')]
    max_words_mutated_percentage = op_ctx.parameters.get('max-words-mutated-percentage')
    mutations_kwargs_overrides = {}
    if levenshtein_dict is not None:
        mutations_kwargs_overrides[mutator.MutationType.CLOSE_LEV_WORD] = {'vocab': levenshtein_dict}
        whitelist.append(mutator.CloseLevWord.name())

    # todo get more kwargs from op_ctx params maybe?
    mt = mutator.Mutator(
        lang=lang,
        mutations_whitelist=whitelist,
        mutations_kwargs_overrides=mutations_kwargs_overrides,
        weight_limit=weight_limit,
        max_words_mutated_percentage=max_words_mutated_percentage,
    )

    mutated_texts = []
    for text in texts:
        mutated_result = mt.mutate(text)
        print('Done:', mutated_result)
        mutated_texts.append({
            'source_text': text,
            'mutated_text': mutated_result.new_text,
            'mutations': [mutation.value for mutation in mutated_result.applied_mutations],
        })

    with open(op_ctx.outputs.get('mutated_texts.json'), 'w') as f:
        json.dump(mutated_texts, f, ensure_ascii=False)
