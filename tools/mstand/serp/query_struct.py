from typing import Optional

import yaqutils.misc_helpers as umisc


@umisc.hash_and_ordering_from_key_method
class QueryKey(object):
    def __init__(self, query_text: str, query_region: int, query_device: int = 0, query_uid: str = None,
                 query_country: Optional[str] = None, query_map_info: Optional[dict] = None):
        self.query_text = umisc.to_unicode(query_text)
        self.query_region = query_region
        self.query_device = query_device
        self.query_uid = umisc.to_string(query_uid)
        self.query_country = query_country
        self.query_map_info = query_map_info

    def serialize(self):
        result = {
            "region": self.query_region,
            "device": self.query_device,
            "text": self.query_text,
            "uid": self.query_uid,
            "country": self.query_country,
            "mapInfo": self.query_map_info
        }
        return result

    @staticmethod
    def deserialize(json_data):
        query_text = json_data["text"]
        query_region = json_data["region"]
        query_device = json_data["device"]
        query_uid = json_data.get("uid")
        query_country = json_data.get("country")
        query_map_info = json_data.get("mapInfo")
        return QueryKey(query_text=query_text,
                        query_region=query_region,
                        query_device=query_device,
                        query_uid=query_uid,
                        query_country=query_country,
                        query_map_info=query_map_info)

    @staticmethod
    def deserialize_external(json_data):
        query_text = json_data["text"]
        query_region = json_data["regionId"]
        query_device = json_data["device"]
        query_uid = json_data.get("uid")
        query_country = json_data.get("country")
        query_map_info = json_data.get("mapInfo")
        return QueryKey(query_text=query_text,
                        query_region=query_region,
                        query_device=query_device,
                        query_uid=query_uid,
                        query_country=query_country,
                        query_map_info=query_map_info)

    def serialize_external(self):
        serialized = {
            "regionId": self.query_region,
            "device": self.query_device,
            "text": self.query_text,
            "uid": self.query_uid,
            "country": self.query_country,
            "mapInfo": self.query_map_info
        }
        return serialized

    def to_tuple(self, dump_settings):
        res = (self.query_text,)
        res += (str(self.query_region),)

        if dump_settings.with_device:
            res += (str(self.query_device),)

        if dump_settings.with_uid:
            res += (str(self.query_uid),)

        if dump_settings.with_country:
            res += (str(self.query_country),)

        # currently, mapInfo is not exported.
        return res

    def __str__(self):
        return "QKey('{}', r={}, d={}, u={}, c={}, mi={})".format(umisc.to_string(self.query_text),
                                                                  self.query_region,
                                                                  self.query_device,
                                                                  self.query_uid,
                                                                  self.query_country,
                                                                  self.query_map_info)

    def key(self):
        # mapInfo is a dict, so it's easier to take str() for key
        # because we don't want to parse mapInfo.
        return (self.query_text,
                str(self.query_region),
                str(self.query_device),
                str(self.query_uid),
                str(self.query_country),
                str(self.query_map_info))


class SerpQueryInfo(object):
    def __init__(self, qid, query_key, is_failed_serp=False):
        """
        :type qid: int
        :type query_key: QueryKey
        """
        self.qid = qid
        self.is_failed_serp = is_failed_serp
        self.query_key = query_key

    def __str__(self):
        return "Query({}, qid={}, is_failed_serp={})".format(self.query_key, self.qid, self.is_failed_serp)

    def serialize(self):
        serialized = {
            "qid": self.qid,
        }
        if self.is_failed_serp:
            serialized["is-failed-serp"] = self.is_failed_serp
        serialized.update(self.query_key.serialize())
        return serialized

    @staticmethod
    def deserialize(json_data):
        qid = json_data["qid"]
        is_failed_serp = json_data.get("is-failed-serp", False)
        query_key = QueryKey.deserialize(json_data)
        return SerpQueryInfo(qid=qid, query_key=query_key, is_failed_serp=is_failed_serp)
