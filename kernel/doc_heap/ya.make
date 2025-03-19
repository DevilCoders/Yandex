LIBRARY()

OWNER(
    andreikkaa
    g:base
)

SRCS(
    array_doc_heap.cpp
    array_doc_heap.h
    list_doc_heap.cpp
    list_doc_heap.h
    smart_doc_heap.cpp
    smart_doc_heap.h
    consumers.cpp
    consumers.h
    relevance_getters.cpp
    relevance_getters.h
)

PEERDIR(
    library/cpp/containers/stack_vector
)

END()
