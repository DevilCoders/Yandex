
def find_not_null(records, field_name):
    value_to_fill = None
    for rec in records:
        field_value = getattr(rec, field_name)
        if field_value is not None:
            value_to_fill = field_value
            break
    return value_to_fill
    
def reduce(keys, records):
    records_list = [rec for rec in records]
    all_fields=['campaign_id', 'campaign_type', 'channel', 'email', 'event', 'letter_code', 
                'letter_id', 'link_url', 'message_id', 'message_type', 'source', 'status', 
                'tags', 'title', 'unixtime'] 
    fields_to_fill = ['campaign_id', 'campaign_type', 'channel', 'email',
                      'letter_code', 'tags', 'title', 'message_type']
    not_null_fields_to_fill = {field: find_not_null(records_list, field) for field in fields_to_fill}
    for rec in records_list:
        res_dict = {field:getattr(rec, field) for field in all_fields}
        for field in fields_to_fill:
            res_dict[field] = not_null_fields_to_fill[field]
        yield res_dict