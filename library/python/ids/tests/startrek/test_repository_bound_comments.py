# -*- coding: utf-8 -*-

import unittest
import pytest

from ids.registry import registry


class TestStartrekCommentsBoundRepository(unittest.TestCase):

    def setUp(self):
        self.token = 'c24ce44d0fc9459f85e3df9d8b79b40c'
        self.link = 'https://st-api.test.yandex-team.ru'
        self.rep = registry.get_repository('startrek', 'issues',
                                           user_agent='ids-test',
                                           oauth2_access_token=self.token,
                                           server=self.link)

        # key: TEST-71
        # id: 504dd03be4b098e57a6b6d32
        # eventId:50606d69e4b05296c42c7ac3
        self.parent = self.rep.get_one(lookup={'key': 'IDS-1'})
        self.comments = self.parent['rep_comments']

        self.startrek = self.rep.STARTREK_CONNECTOR_CLASS(self.rep.options)


    @pytest.mark.integration
    def test_getting_from_parent_resource(self):
        comment_id = "50ffeeb3e4b0ae7a9a3f1998"
        comment = self.comments.get_one(lookup={"comment_id": comment_id})
        self.assertEquals(comment_id, comment["id"])

    @pytest.mark.integration
    def test_creating(self):
        comment_text = ("ids.startrek.tests.TestStartrekCommentsBoundRepository"
                        ".test_creating\nhttps://github.yandex-team.ru/tools/id"
                        "s/blob/master/packages/startrek/tests/repository_bound"
                        "_comments.py")
        fields = {"text": comment_text}
        comment = self.comments.create(fields)
        self.assertEquals(comment_text, comment["text"])

    @pytest.mark.integration
    def test_updating(self):
        comment_text = ("ids.startrek.tests.TestStartrekCommentsBoundRepository"
                        ".test_updating\nhttps://github.yandex-team.ru/tools/id"
                        "s/blob/master/packages/startrek/tests/repository_bound"
                        "_comments.py")
        comment = self.comments.create({"text": comment_text})

        new_comment_text = ("!!**UPDATED**!!: ids.startrek.tests.TestStartrekCo"
                            "mmentsBoundRepository.test_updating\nhttps://githu"
                            "b.yandex-team.ru/tools/ids/blob/master/packages/st"
                            "artrek/tests/repository_bound_comments.py")
        self.comments.update(comment, {"text": new_comment_text})
        updated = self.comments.get_one(lookup={"comment_id": comment["id"]})

        self.assertEquals(new_comment_text, comment["text"])
        self.assertEquals(new_comment_text, updated["text"])
