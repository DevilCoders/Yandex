LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    ann_data_hit.h
    attributes_hit.h
    counts_hit.h
    doc_attrs_hit.h
    long_form_hit.h
    panther_hit.h
    superlong_hit.h
    uninverted_hit.h
    click_sim_hit.h
    dummy.cpp
    sent_len_hit.h
)

PEERDIR(
    kernel/doom/enums
    kernel/indexann/hit
    library/cpp/wordpos
)

END()
