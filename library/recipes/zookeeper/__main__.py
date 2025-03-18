from library.python.testing.recipe import declare_recipe
from library.recipes.zookeeper.lib import start, stop


if __name__ == '__main__':
    declare_recipe(start, stop)
