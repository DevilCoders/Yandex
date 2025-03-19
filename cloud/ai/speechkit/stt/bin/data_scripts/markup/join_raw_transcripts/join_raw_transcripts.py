from collections import defaultdict
import json
import os
import typing

import nirvana.job_context as nv


# Скрипт делался для фрагментов записей КЦ-288, поэтому рассматриваем старый формат URL-ов.
# Кроме того, закладываемся на старый input/output формат склейщика текстов.
def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs

    with open(inputs.get('url_to_text.json')) as f:
        url_to_text: typing.Dict[str, str] = json.load(f)

    record_id_to_bits = defaultdict(list)
    for url, text in url_to_text.items():
        key = url[len('https://storage.yandexcloud.net/cloud-ai-data/'):]
        bit_name, _ = os.path.splitext(os.path.basename(key))
        record_id, rng = bit_name.split('_')
        start_ms, end_ms = [int(ms) for ms in rng.split('-')]
        record_id_to_bits[record_id].append((start_ms, text))

    record_id_to_sorted_bits = {}
    for record_id, bits in record_id_to_bits.items():
        sorted_bits = sorted(bits, key=lambda bit: bit[0])
        record_id_to_sorted_bits[record_id] = sorted_bits

    joiner_in = []
    for record_id, bits in record_id_to_sorted_bits.items():
        joiner_in.append(f'{len(bits)} {record_id}')
        for start_ms, text in bits:
            joiner_in.append(f'{start_ms} {text}')

    input_path = 'joiner_in.txt'
    output_path = 'joiner_out.txt'

    with open(input_path, 'w') as f:
        f.write('\n'.join(joiner_in))

    exit_code = os.system(f'{inputs.get("markup_joiner_executable")} --input {input_path} '
                          f'--bit_offset 3000 --output {output_path}')

    if exit_code != 0:
        raise RuntimeError(f'Markup joiner executable finished with exit code {exit_code}')

    with open(output_path) as f:
        joiner_out = f.read()

    record_id_to_text = {}
    for line in joiner_out.strip().split('\n'):
        record_id, text = line.split(' ', 1)
        record_id_to_text[record_id] = [text.strip()]  # multichannel calc metrics format

    with open(outputs.get('record_id_to_text.json'), 'w') as f:
        json.dump(record_id_to_text, f, indent=4, ensure_ascii=False)
