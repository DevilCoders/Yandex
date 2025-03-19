import argparse
import random
from urllib import request

parser = argparse.ArgumentParser(description='Generate MTD server ammo')

parser.add_argument('-s', '--source', type=str, default='ru-en:text.txt', help="""
Ammo source, comma-separated list of language directions and text files
with source texts. For example:

ru-en:dostoevsky_besy.txt,fr-en:duma_les_trois_mousquetaires.txt,en-ru:shakespeare_the_merchant_of_venice.txt

Each source file will be splitted by senteces and this sentences will
be randomly sampled along with language direction.

So, for example, during sampling we can get this sequence of requests:
en-ru:<sentence 34>
ru-en:<sentence 2>
ru-en:<sentence 817>
fr-en:<sentence 100>
...

and so on.
""")
parser.add_argument('-c', '--sentence_count', type=int, default='100', help="""
Number of randomly sampled direction-sentence.
""")

srv = 'volume'
options = '32'  # No cache, see https://a.yandex-team.ru/arc/trunk/arcadia/dict/mt/libs/tr/translator.h?rev=5216080#L41

args = parser.parse_args()

directions_with_source_files = args.source.split(',')
direction_to_sentences = {}

for s in directions_with_source_files:
    direction, source_file = s.split(':')
    with open(source_file, 'r') as f:
        text = f.read()
        sentences = text.split('.')
        sentences = [s.strip() for s in sentences if len(s) > 0]
        direction_to_sentences[direction] = sentences

body_template = 'lang={lang}&srv={srv}&options={options}&text={text}'

with open('ammo.txt', 'w') as f:
    f.write('[Content-Type: application/x-www-form-urlencoded]\n')

    for _ in range(args.sentence_count):
        direction = random.choice(list(direction_to_sentences.keys()))
        sentence = random.choice(direction_to_sentences[direction])

        body = body_template.format(
            lang=direction,
            srv=srv,
            options=options,
            text=request.quote(sentence))

        f.write('%d /api/v1/tr.json/translate' % len(body))
        f.write('\n')
        f.write(body)
        f.write('\n')
