import random
import string
import os
import argparse
from collections import namedtuple
from urlparse import urljoin
from jinja2 import Template

import numpy as np
import matplotlib.image as mpimg

from antiadblock.cryprox.load_test.stub_generator.ammo_generator import generate_ammo


DEFAULT_IMG_PATHS = (
    'https://awaps.yandex.net',
    '//avatars.mds.yandex.net/get-canvas/',
    '//avatars.mds.yandex.net/get-direct/',
    '//avatars.mds.yandex.net/get-rtb/',
)
IMAGES_SUBDIR = ['get-rtb', 'get-canvas', 'get-direct']
DEFAULT_IMG_SIZES = ((100, 100), (120, 100), (150, 100), (170, 100), (200, 100))
DEFAULT_PAGE_SIZE = 250
DEFAULT_IMGS_COUNT = 5
DEFAULT_STUBS = 'stubs'
GENERATED_STUBS = 'stubs/generated'


def generate_images(imgs, path):
    """
    Generate images and save them
    :param imgs: list with img objects to generate. Format: namedtuple('Image', 'name size')
    :param path: absolute path to save generated imgs
    """
    for i in IMAGES_SUBDIR:
        if not os.path.exists(os.path.join(path, i)):
            os.makedirs(os.path.join(path, i))

    for image in imgs:
        mpimg.imsave(os.path.join(path, image.name), np.random.rand(*image.size), format='png')
        for i in IMAGES_SUBDIR:
            os.link(os.path.join(path, image.name), os.path.join(path, i, image.name))


def _img_name(extension='png', size=(0, 0)):
    """
    Simple random image file name generator
    :param extension: image extension
    :return: random image file name
    """
    return '{name}_{s[0]}x{s[1]}.{ext}'.format(name=''.join([random.choice(string.letters.lower()) for _ in xrange(10)]), s=size, ext=extension)


def generate_stubs(path, page_size, imgs_count):
    if imgs_count < 1:
        raise ValueError('Number of images within page can`t be less than 1!')
    if imgs_count < len(DEFAULT_IMG_SIZES):
        raise ValueError('Number of images can`t be less than number of images sizes!')

    if not os.path.exists(path):
        os.makedirs(path)

    Img = namedtuple('Img', 'name size')
    guarantee_images = [Img(name=_img_name(size=size), size=size) for size in DEFAULT_IMG_SIZES]
    random_sizes = [random.choice(DEFAULT_IMG_SIZES) for _ in xrange(imgs_count - len(guarantee_images))]
    images = guarantee_images + [Img(name=_img_name(size=size), size=size) for size in random_sizes]

    page = PageGenerator(page_size, images)
    with open(os.path.join(path, 'load.html'), 'w') as load_html:
        load_html.write(page.layout())

    generate_images(images, path)


class PageGenerator(object):
    """
    Class for html page layout generating
    Allow to set number of img tags and size of page
    """
    def __init__(self, page_size, images):
        """
        :param page_size (int): desired page size in kb
        :param images (list): list with images objects to put into layout.
        """

        self.page_size = page_size
        self.images = images

        self.basic_layout = Template("""
<html>
<head>
<title>Template page</title>
</head>
<body>
    {{ body }}
</body>
</html>

""")

    def _imgs(self):
        """
        Gerenate N random `<img>` html tags with images, where N = len(self.images)
        :return: list with generated tags
        """
        img_tag = Template('<img src="{{ img_path }}" />')
        return [img_tag.render(img_path=urljoin(random.choice(DEFAULT_IMG_PATHS), image.name)) for image in self.images]

    def _texts(self, size):
        """
        Generate `<p>` tags with content to fill the page to the desired size
        :param size (int): total desired texts size in Kb
        :return: list with generated tags
        """
        text_tag = Template('<script type="text/javascript">{{ text }}</script>')
        # Default one text block (`p` tag) size in kb
        block_size = 10
        # Random text block generator
        return [text_tag.render(text=''.join([random.choice(string.letters) for _ in xrange(block_size * 1024)])) for _ in xrange(int(size / block_size))]

    def layout(self):
        """
        Create page layout by rendering basic layout with shuffled imgs and texts blocks
        :return: html layout
        """
        image_tags = self._imgs()
        # calculate size of block with img tags in kb
        image_tags_size = int(sum((len(img_block) for img_block in image_tags)) / 1024)
        if self.page_size < image_tags_size:
            raise ValueError('Desired page size less than `<img />` tags total size! '
                             'Desired page size: {} Kb, Total img tags size: {} Kb'.format(self.page_size, image_tags_size))

        text_tags = self._texts(self.page_size - image_tags_size)
        # merge and shuffle tags
        content = '\n'.join(sorted(image_tags + text_tags, key=lambda k: random.random()))
        return self.basic_layout.render(body=content)


def get_key_and_token(service_id='test'):
    crypt_key = 'duoYujaikieng9airah4Aexai4yek4qu'
    token = ('eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1NjE2NTgwNTcsInN1YiI6InRlc3QiLCJleHAiOjE1OTMyMDQ4NTd9.p_y1BaJDwnUSjjKr_8Ms709YW2krM6P51IXhh2irKE7GX-DJiEElU22kHR9Mfx6y'
             'XhcHSfyUb31O_ejZ2MMMkjDV9cGDA-c_Q8XNJ9zu6R31zg_t9QrhHjsdtq8kJpTRFfAl-A8iOkH5zNuryez1beeXjz0DIoKkBxWbaDh5-y5hcA-b0uJuV2KUoTERtRKklJPAbf7XzEVEZp196z46krPbN_wgug5IhLABFGbl72EkMgUCZZZ3gn7'
             'qV-AfsCPHqkeQ56G6DOG9R-2xj2QXUTS-5GwHMSG1BxyZNrS7taXr0dxiZjlwbze3-2HA80-EfswkKZD1apgSW3Y8huBOxA')
    return crypt_key, token


def main():
    parser = argparse.ArgumentParser(description='Tool for generating html page with content (images & text)')
    parser.add_argument('--page_size', default=DEFAULT_PAGE_SIZE,
                        metavar='page_size',
                        help='Desired page size in kb')
    parser.add_argument('--imgs_count', default=DEFAULT_IMGS_COUNT,
                        metavar='imgs_count',
                        help='Images count to generate')

    args = parser.parse_args()

    generate_stubs(GENERATED_STUBS, int(args.page_size), int(args.imgs_count))
    crypt_key, token = get_key_and_token()
    ammo, stats = generate_ammo(DEFAULT_STUBS, crypt_key, token)
    with open(os.path.join(DEFAULT_STUBS, 'load_ammo.txt'), 'w') as f:
        f.write(ammo)
    with open(os.path.join(DEFAULT_STUBS, 'description.txt'), 'w') as f:
        f.write(stats)


if __name__ == "__main__":
    main()
