# coding: utf-8

from tools.releaser.src.lib import https
from tools.releaser.src.conf import cfg


class DockerInfoException(Exception):
    def __init__(self, status_code, error_message):
        self.status_code = status_code
        self.error_message = error_message

    def __str__(self):
        return '%d: %s' % (self.status_code, self.error_message)


class DockerInfoClient(object):
    def __init__(self, oauth_token):
        """
        :type oauth_token: str
        """
        self.oauth_token = oauth_token

    def get_image_hash(self, image_url_without_tag, image_tag):
        """
        :type image_url_without_tag: str
        :type image_tag: str
        :rtype: unicode
        """
        session = https.get_internal_session()
        response = session.get(
            '%s/api/docker/hash?registryUrl=%s&tag=%s' % (
                cfg.DOCKER_INFO_URL, image_url_without_tag, image_tag
            ),
            headers={
                'Authorization': 'OAuth %s' % self.oauth_token
            },
        )

        if response.status_code != 200:
            raise DockerInfoException(response.status_code, response.content)

        return response.text
