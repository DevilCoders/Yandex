import yaqutils.misc_helpers as umisc


class SerpMarkupInfo(object):
    def __init__(self, qid, res_markups=None, serp_data=None):
        """
        :type qid: int
        :type res_markups: list[ResMarkupInfo] | None
        :type serp_data: dict | None
        """
        if res_markups is not None and not isinstance(res_markups, list):
            raise Exception("Incorrect type of 'res_markups' parameter (expected list or None)")
        if serp_data is not None and not isinstance(serp_data, dict):
            raise Exception("Incorrect type of 'serp_data' parameter (expected dict or None)")

        self.qid = qid
        self.res_markups = res_markups or []
        # dict with metric values and other SERP-level requirements
        self.serp_data = serp_data or {}

    def result_count(self):
        return len(self.res_markups)

    def serialize(self):
        """
        :rtype: dict
        """
        serialized = {
            "qid": self.qid
        }
        if self.res_markups:
            serialized["markups"] = umisc.serialize_array(self.res_markups)
        if self.serp_data:
            serialized["serp_data"] = self.serp_data
        return serialized

    @staticmethod
    def deserialize(json_data):
        """
        :type json_data: dict
        :rtype: SerpMarkupInfo
        """
        qid = json_data["qid"]
        # do not deserialize old-style markup
        assert "results" not in json_data
        ser_markups = json_data.get("markups", [])
        results = umisc.deserialize_array(ser_markups, ResMarkupInfo)
        serp_data = json_data.get("serp_data", {})
        return SerpMarkupInfo(qid=qid, res_markups=results, serp_data=serp_data)


class ResMarkupInfo(object):
    def __init__(self, pos, related_query_text=None, is_wizard=False, wizard_type=None,
                 scales=None, site_links=None, is_fast_robot_src=False, coordinates=None,
                 alignment=None, result_type=None, json_slices=None):
        """
        :type pos: int | None
        :type related_query_text: str | None
        :type is_wizard: bool
        :type wizard_type: str | None
        :type scales: dict[str] None
        :type site_links: list[dict] | None
        :type is_fast_robot_src: bool
        :type coordinates: dict | None
        :type alignment: int | None
        :type result_type: int | None
        :type json_slices: list[str] | None
        """
        if related_query_text is None and pos is None:
            raise Exception("Bad ResMarkupInfo: neither related_query_text, nor pos are set.")
        self.pos = pos
        self.related_query_text = related_query_text
        self.is_wizard = is_wizard
        self.wizard_type = wizard_type
        self.scales = scales or {}
        self.site_links = site_links or []
        self.is_fast_robot_src = is_fast_robot_src
        self.coordinates = coordinates
        self.alignment = alignment
        # aka componentInfo.type in Metrics
        self.result_type = result_type
        self.json_slices = json_slices or []

    def serialize(self):
        """
        :rtype: dict
        """
        serialized = {}
        if self.pos is not None:
            serialized["pos"] = self.pos
        if self.scales:
            serialized["scales"] = self.scales
        if self.site_links:
            serialized["siteLinks"] = self.site_links
        if self.related_query_text:
            serialized["relQueryText"] = self.related_query_text
        if self.is_wizard:
            serialized["isWizard"] = self.is_wizard
        if self.wizard_type is not None:
            serialized["wizardType"] = self.wizard_type
        if self.is_fast_robot_src:
            serialized["isFastRobotSrc"] = self.is_fast_robot_src
        if self.coordinates:
            serialized["coordinates"] = self.coordinates
        if self.alignment is not None:
            serialized["alignment"] = self.alignment
        if self.result_type is not None:
            serialized["resultType"] = self.result_type
        if self.json_slices:
            serialized["jsonSlices"] = self.json_slices
        return serialized

    @staticmethod
    def deserialize(json_data):
        """"
        :type json_data: dict
        :rtype: ResMarkupInfo
        """
        pos = json_data.get("pos")
        alignment = json_data.get("alignment")
        related_query_text = json_data.get("relQueryText")
        scales = json_data.get("scales")
        site_links = json_data.get("siteLinks")
        is_wizard = json_data.get("isWizard", False)
        wizard_type = json_data.get("wizardType")
        is_fast_robot_src = json_data.get("isFastRobotSrc", False)
        coordinates = json_data.get("coordinates")
        result_type = json_data.get("resultType")
        json_slices = json_data.get("jsonSlices")
        return ResMarkupInfo(pos=pos, related_query_text=related_query_text, is_wizard=is_wizard,
                             wizard_type=wizard_type, scales=scales, site_links=site_links,
                             is_fast_robot_src=is_fast_robot_src, coordinates=coordinates,
                             alignment=alignment, result_type=result_type, json_slices=json_slices)


class SerpUrlsInfo(object):
    def __init__(self, qid, res_urls=None):
        """
        :type qid: int
        :type res_urls: list[ResUrlInfo] | None
        """
        if res_urls is not None and not isinstance(res_urls, list):
            raise Exception("Incorrect type of 'res_markups' parameter (expected list or None)")

        self.qid = qid
        self.res_urls = res_urls or []
        # dict with metric values and other SERP-level requirements

    def serialize(self):
        """
        :rtype: dict
        """
        serialized = {
            "qid": self.qid
        }
        if self.res_urls:
            serialized["urls"] = umisc.serialize_array(self.res_urls)
        return serialized

    @staticmethod
    def deserialize(json_data):
        """
        :type json_data: dict
        :rtype: SerpUrlsInfo
        """
        qid = json_data["qid"]
        assert "results" not in json_data
        assert "markups" not in json_data
        ser_urls = json_data.get("urls", [])
        res_urls = umisc.deserialize_array(ser_urls, ResUrlInfo)
        return SerpUrlsInfo(qid=qid, res_urls=res_urls)


class ResUrlInfo(object):
    def __init__(self, url, pos, is_turbo=False, is_amp=False, original_url=None):
        """
        :type url: str | None
        :type pos: int | None
        """
        self.url = url
        self.pos = pos
        self.is_turbo = is_turbo
        self.is_amp = is_amp
        self.original_url = original_url

    def serialize(self):
        """
        :rtype: dict
        """
        serialized = {}
        if self.url:
            serialized["url"] = self.url
        if self.pos is not None:
            serialized["pos"] = self.pos
        if self.is_turbo:
            serialized["isTurbo"] = self.is_turbo
        if self.is_amp:
            serialized["isAmp"] = self.is_amp
        if self.original_url:
            serialized["originalUrl"] = self.original_url

        return serialized

    @staticmethod
    def deserialize(json_data):
        """"
        :type json_data: dict
        :rtype: ResUrlInfo
        """
        url = json_data.get("url")
        pos = json_data.get("pos")
        is_turbo = json_data.get("isTurbo", False)
        is_amp = json_data.get("isAmp", False)
        original_url = json_data.get("originalUrl")
        return ResUrlInfo(url=url, pos=pos, is_turbo=is_turbo, is_amp=is_amp, original_url=original_url)
