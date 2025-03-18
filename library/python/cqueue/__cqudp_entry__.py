import sys
import base64
import pickle

try:
    res = pickle.loads(base64.b64decode(sys.argv[1]))(), None
except Exception as e:
    res = None, e

sys.stdout.write(pickle.dumps(res))
sys.stdout.flush()
