import random
import string
from time import gmtime, strftime


def generate_random_string(length):
    return "".join([random.choice(string.ascii_lowercase) for _ in range(length)])


def get_current_datetime_string():
    return strftime("%Y_%m_%d__%H_%M_%S", gmtime())
