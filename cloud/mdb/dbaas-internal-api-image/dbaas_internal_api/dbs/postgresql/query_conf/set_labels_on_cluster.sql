SELECT code.set_labels_on_cluster(
    i_folder_id  => %(folder_id)s,
    i_cid        => %(cid)s,
    i_labels     => %(labels)s::code.label[],
    i_rev        => %(rev)s
)