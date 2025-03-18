import logging

from serp import ParsedSerpDataStorage


class DumpSettings(object):
    def __init__(self, with_serpset_id, query_mode, with_qid, with_device,
                 with_uid, with_country, with_position, serp_depth):
        """
        :type with_serpset_id: bool
        :type query_mode: bool
        :type with_qid: bool
        :type with_device: bool
        :type with_uid: bool
        :type with_country: bool
        :type with_position: bool
        :type serp_depth: int | None
        """
        if query_mode and with_position:
            raise Exception("Query-only mode is incompatible with position export.")

        self.with_serpset_id = with_serpset_id
        self.query_mode = query_mode
        self.with_device = with_device
        self.with_qid = with_qid
        self.with_uid = with_uid
        self.with_country = with_country
        self.with_position = with_position
        self.serp_depth = serp_depth

    def __str__(self):
        fields = ["text", "region"]

        if self.with_device:
            fields.append("device")
        if self.with_uid:
            fields.append("uid")
        if self.with_country:
            fields.append("country")

        if not self.query_mode:
            fields.append("url")

        if self.with_position:
            fields.append("pos")

        if self.with_qid:
            fields.append("qid")

        if self.with_serpset_id:
            fields.append("serpset-id")

        qmode = "YES" if self.query_mode else "NO"
        return "DumpSettings: query mode: {}, query fields: {}".format(qmode, ":".join(fields))

    def __repr__(self):
        return str(self)


class ExtendSettings(object):
    def __init__(self, query_mode, with_position, overwrite, flat_mode, field_name):
        """
        :type query_mode: bool
        :type with_position: bool
        :type overwrite: bool
        :type flat_mode: bool
        :type field_name: str | None
        """
        if query_mode and with_position:
            raise Exception("Cannot use query-only mode together with position key matching. ")

        if flat_mode:
            if field_name:
                logging.info("In FLAT extend mode field name is used as a prefix.")
        else:
            if not field_name:
                raise Exception("Field name could not be empty in non-flat mode. ")

        if query_mode:
            logging.info("Using query-only mode")
        else:
            logging.info("Using query+URL mode")

        self.query_mode = query_mode
        self.with_position = with_position
        self.overwrite = overwrite
        self.flat_mode = flat_mode
        self.field_name = field_name or ""

    def __str__(self):
        return "ExtendSett(qm={qm}, wp={wp}, ow={ow}, fm={fm}, fn={fn})".format(qm=self.query_mode,
                                                                                wp=self.with_position,
                                                                                ow=self.overwrite,
                                                                                fm=self.flat_mode,
                                                                                fn=self.field_name)

    def __repr__(self):
        return str(self)


class FieldExtenderContext(object):
    def __init__(self, parsed_serp_storage, field_values, extend_settings, extender=None, threads=1):
        """
        :type parsed_serp_storage: ParsedSerpDataStorage
        :type field_values: dict | None
        :type extend_settings: ExtendSettings
        :type extender:
        :type threads: int
        """
        assert isinstance(parsed_serp_storage, ParsedSerpDataStorage)
        assert isinstance(extend_settings, ExtendSettings)

        self.parsed_serp_storage = parsed_serp_storage
        self.field_values = field_values or {}
        self.extend_settings = extend_settings
        self.extender = extender
        self.threads = threads
