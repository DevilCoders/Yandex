import sys

cluster_id = '{cluster_id}'


def script(inputs, *_, **__):
    with open(inputs[0], 'r') as f:
        tables = [line.strip() for line in f]

    query = ''
    for line in tables:
        query += ' '.join(['DROP TABLE IF EXISTS', line, 'ON CLUSTER', cluster_id, '; \\n'])
    sys.stdout.write(query)
