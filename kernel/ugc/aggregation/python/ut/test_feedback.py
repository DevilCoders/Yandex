# coding=utf-8
from kernel.ugc.aggregation.python import FeedbackReader, AppendOptions
import library.python.resource as rs
import json
from pytest import raises


def json_dumps(obj):
    return json.dumps(obj, ensure_ascii=False)


def test_append_feedback():
    fr = FeedbackReader()
    data = json.loads(rs.find("/data1/updates"))
    # unicode test
    data[1]["objects"][0]["opinionText"] = u"хорошо"
    for a in data:
        assert fr.append_feedback(json_dumps(a))

    assert fr.value("objects", "o1", "opinionText") == u"хорошо"
    props = fr.get_object_props("objects", "o1")
    assert props is not None
    assert "iLikeIt" in props
    assert props["iLikeIt"].value == "true"
    assert props["iLikeIt"].time == 1234567890
    assert "opinionText" in props
    assert props["opinionText"].value == u"хорошо"

    props = fr.get_object_props("objects", "unknown_key")
    assert props is None


def test_append_feedback_failed():
    fr = FeedbackReader()
    data = json.loads(rs.find("/data1/updates"))
    del data[0]["type"]
    with raises(Exception):
        fr.append_feedback(json_dumps(data[0]))


def test_load_from_user_data():
    userdata = {}
    userdata["type"] = "user-data"
    for update in json.loads(rs.find("/data1/updates")):
        new_update = {}
        new_update["value"] = json_dumps(update)
        new_update["app"] = update["app"]
        new_update["time"] = update["time"]
        userdata["updates"] = [new_update] + userdata.get("updates", [])

    fr = FeedbackReader()
    res = fr.load_from_user_data(json_dumps(userdata))
    assert res == len(userdata["updates"])

    result = json.loads(rs.find("/data1/result"))
    assert fr.aggregate(False) == result[0]
    assert fr.aggregate(True) == result[1]


def test_custom_table_key():
    fr = FeedbackReader("parentId")
    data = json.loads(rs.find("/data7/updates"))
    for a in data:
        assert fr.append_feedback(json_dumps(a))

    result = json.loads(rs.find("/data7/result"))
    assert fr.aggregate() == result


def test_get_object_review_time():
    fr = FeedbackReader()
    data = json.loads(rs.find("/data6/updates"))
    options = AppendOptions(add_reviews_time=True)
    for a in data:
        assert fr.append_feedback(json_dumps(a), options)

    review_time = fr.get_object_review_time("/sprav/1018907821")
    assert review_time == 1506690080454
    rating_time = fr.get_object_rating_time("/sprav/1018907821")
    assert rating_time == 1506690080454


def make_object_update(time, key, col, text, meta={}):
    update = dict()
    update["userId"] = "/user/123"
    update["version"] = "1.0"
    update["app"] = "serp"
    update["type"] = "ugcupdate"
    update["time"] = time
    update["objects"] = [{}]
    update["objects"][0]["key"] = key
    update["objects"][0][col] = text
    for item in meta:
        update[item[0]] = item[1]
    return update


def test_add_photos():
    fr = FeedbackReader()
    options = AppendOptions(add_photos=True)
    updates = [
        make_object_update(123456789010, "/sprav/1001", "photoId", "1"),
        make_object_update(123456789020, "/sprav/1001", "photoId", "2")
    ]
    for update in updates:
        assert fr.append_feedback(json_dumps(update), options)

    photos = fr.get_object_photos("/sprav/1001")
    assert photos == ["1", "2"]

    photos = fr.get_object_photos("/sprav/1002")
    assert photos == []


def test_add_meta():
    fr = FeedbackReader()
    options = AppendOptions(add_meta=True, add_reviews_time=True)
    updates = [
        make_object_update(123456789000, "/sprav/1001", "reviewOverall", "test_review"),
        make_object_update(123456789010, "/sprav/1001", "reviewOverall", "test_review", [("yandexTld", "en")]),
        make_object_update(123456789020, "/sprav/1001", "reviewOverall", "test_review", [("countryCode", "157")]),
        make_object_update(123456789040, "/sprav/1001", "reviewOverall", "test_review_updated",
                            [("updateId", "new_update_id"), ("yandexTld", "ru"), ("countryCode", "235")]),
        make_object_update(123456789030, "/sprav/1001", "reviewOverall", "test_review", [("updateId", "old_update_id")])
    ]
    for update in updates:
        assert fr.append_feedback(json_dumps(update), options)

    review_meta = fr.get_object_review_meta("/sprav/1001")
    assert review_meta.YandexTLD == "ru"
    assert review_meta.CountryId == "235"
    assert review_meta.UpdateId == "new_update_id"

    review_meta = fr.get_object_review_meta("/sprav/1002")
    assert review_meta.YandexTLD == ""
    assert review_meta.CountryId == ""
    assert review_meta.UpdateId == ""
