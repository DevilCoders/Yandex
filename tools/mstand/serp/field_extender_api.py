class ExtendParamsForAPI(object):
    def __init__(self, scales, field_name, custom_value, do_overwrite, pos):
        self.scales = scales
        # field name or prefix
        self.field_name = field_name
        self.custom_value = custom_value
        self.do_overwrite = do_overwrite
        self.pos = pos
