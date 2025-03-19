SELECT code.set_maintenance_window_settings(
    i_cid         => %(cid)s,
    i_day         => %(day)s,
    i_hour        => %(hour)s,
    i_rev         => %(rev)s
)
