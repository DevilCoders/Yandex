import json
import string
import random
import uhashring as hash_ring


def rnd_string(size=32, chars=string.ascii_lowercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


def ring(partitions=10):
    nodes = ['realtime.{}'.format(i) for i in range(partitions)]
    return hash_ring.HashRing(nodes=nodes, hash_fn="ketama")


def main():
    for parts in range(2, 10, 2):
        rng = ring(parts)
        for strlen in (10, 20, 30, 40, 50):
            for x in range(100):
                str_key = rnd_string(strlen)
                node = rng.get_node(str_key)
                print(json.dumps({"key": str_key, "parts": parts, "node": node}, sort_keys=True))

if __name__ == "__main__":
    main()
