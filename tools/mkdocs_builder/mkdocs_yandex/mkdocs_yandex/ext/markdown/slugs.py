from slugify import slugify as python_slugify


def slugify(value, separator):
    return python_slugify(value, word_boundary=True, separator=separator, save_order=True)
