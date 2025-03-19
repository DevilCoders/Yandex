LIBRARY()

OWNER(
    alex-sh
    druxa
)

PEERDIR(
    kernel/lemmas_merger
    kernel/ethos/lib/logistic_regression
    kernel/ethos/lib/naive_bayes
    kernel/ethos/lib/out_of_fold
    kernel/ethos/lib/reg_tree
    kernel/ethos/lib/util
    library/cpp/getopt/small
    library/cpp/svnversion
    library/cpp/string_utils/old_url_normalize
    library/cpp/string_utils/url
)

SRCS(
    binary_classifier.h
    binary_classifier.cpp

    binary_model.h

    classifier_features.h
    classifier_features.cpp

    compositions.h
    compositions.cpp

    document.h
    document_factory.h
    document_factory.cpp

    multi_classifier.h
    multi_classifier.cpp
    multi_model.h

    options.h
    util.h
    util.cpp

    tagged_models/model_1562208.h
    tagged_models/options_1562208.h

    tagged_models/model_1562208.cpp
)

GENERATE_ENUM_SERIALIZATION(options.h)
GENERATE_ENUM_SERIALIZATION(tagged_models/options_1562208.h)

END()
