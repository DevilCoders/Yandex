from library.python.testing.deprecated import setup_environment
from library.python.testing.recipe import declare_recipe


def start(argv):
    setup_environment.setup_bin_dir(flatten_all_data=True)


def stop(argv):
    pass


if __name__ == "__main__":
    declare_recipe(start, stop)
