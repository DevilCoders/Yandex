LIBRARY()

OWNER(g:facts)

SRCS(
    classifier_base.cpp
    classifier_data.cpp
    online_alias_classifier.cpp
    similar_fact_classifier.cpp
)

PEERDIR(
    kernel/facts/factors_info
    kernel/facts/features_calculator
    kernel/matrixnet
)

END()
