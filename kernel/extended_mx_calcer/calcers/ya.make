LIBRARY()

OWNER(
    epar
    g:blender
)

SRCS(
    bundle.cpp
    combinations.cpp
    dcg_boost.cpp
    meta.cpp
    random.cpp
    thompson_sampling_distributions.cpp

    GLOBAL thompson_sampling_bundle.cpp
    GLOBAL any_with_filter.cpp
    GLOBAL any_with_random.cpp
    GLOBAL clickint.cpp
    GLOBAL clickregtree.cpp
    GLOBAL factor_filter.cpp
    GLOBAL incut_mix.cpp
    GLOBAL majority_vote.cpp
    GLOBAL multibundle.cpp
    GLOBAL multibundle_by_factors.cpp
    GLOBAL multibundle_with_external_vt_mx.cpp
    GLOBAL multiclass_with_filter.cpp
    GLOBAL multifeature_softmax.cpp
    GLOBAL multi_show.cpp
    GLOBAL mx_with_meta.cpp
    GLOBAL neural_net.cpp
    GLOBAL one_position.cpp
    GLOBAL perceptron.cpp
    GLOBAL positional_binary_composition.cpp
    GLOBAL positional_softmax.cpp
    GLOBAL positional_softmax_positional_subtargets.cpp
    GLOBAL simple.cpp
    GLOBAL surplus_predict.cpp
    GLOBAL two_calcers_union.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/containers/safe_vector
    library/cpp/linear_regression
    library/cpp/scheme
    library/cpp/expression
    library/cpp/perceptron
    library/cpp/string_utils/base64
    kernel/dssm_applier/nn_applier/lib
    kernel/formula_storage
    kernel/extended_mx_calcer/interface
    kernel/extended_mx_calcer/multipredict
    kernel/extended_mx_calcer/factory
    kernel/extended_mx_calcer/proto
    quality/functionality/rtx/lib/beta
)

GENERATE_ENUM_SERIALIZATION(thompson_sampling_distributions.h)
GENERATE_ENUM_SERIALIZATION(thompson_sampling_bundle.h)

END()
