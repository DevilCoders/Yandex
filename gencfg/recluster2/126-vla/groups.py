import argparse

import non_moveable

optimized = {
    'VLA_IMGS_BASE',
    'VLA_WEB_PLATINUM_JUPITER_BASE',
    'VLA_WEB_TIER0_JUPITER_BASE',
    'VLA_VIDEO_PLATINUM_BASE',
    'VLA_VIDEO_TIER0_BASE',
}

# order does matter
non_optimized = [
# builders
   'VLA_WEB_TIER1_BUILD',
   'VLA_WEB_TIER0_BUILD',
   'VLA_IMGS_TIER1_BUILD',
   'VLA_IMGS_TIER0_BUILD',
   'VLA_CALLISTO_BUILD',
   'VLA_VIDEO_TIER0_BUILD',
   'VLA_VIDEO_PLATINUM_BUILD',
   'VLA_WEB_TIER1_EMBEDDING_BUILD',
   'VLA_WEB_TIER1_INVERTED_INDEX_BUILD',
   'VLA_CASTOR_BUILD',
   'VLA_GEMINI_BUILD',
   'VLA_IMGS_NIGHTLY_BUILD',
   'VLA_IMGS_RIM_3K_BUILD',
   'VLA_IMTUB_LARGE_BUILD',
   'VLA_IMTUB_WIDE_BUILD',
   'VLA_MOTHER_BUILD',
   'VLA_WEB_JUD_BUILD',
   'VLA_WEB_MMETA_BUILD',
   'VLA_WEB_PLATINUM_BUILD',

# basesearches
# web prod
   'VLA_WEB_TIER1_EMBEDDING',
   'VLA_WEB_TIER1_INVERTED_INDEX',
   'VLA_WEB_TIER1_JUPITER_BASE',
   'VLA_WEB_TIER1_JUPITER_BASE_HAMSTER',
   'VLA_WEB_TIER1_JUPITER_BASE_MULTI',
   'VLA_WEB_TIER1_REMOTE_STORAGE_BASE',
   'VLA_WEB_CALLISTO_CAM_BASE',
   'VLA_WEB_CALLISTO_CAM_BASE_HAMSTER',
   'VLA_WEB_GEMINI_BASE',
   'VLA_WEB_JUD_JUPITER_BASE_NIDX',

# imgs prod
   'VLA_IMGS_T1_BASE',
   'VLA_IMGS_T1_BASE_HAMSTER',
   'VLA_IMGS_T1_BASE_NIDX',
   'VLA_IMGS_T1_CBIR_BASE',
   'VLA_IMGS_T1_CBIR_BASE_HAMSTER',
   'VLA_IMGS_T1_CBIR_BASE_NIDX',
   'VLA_IMGS_RIM',
   'VLA_IMGS_RIM_3K',
   'VLA_IMGS_RIM_3K_DEPLOY',
   'VLA_IMGS_RIM_3K_PRIEMKA',
   'VLA_IMGS_RIM_3K_PRIEMKA_DEPLOY',
   'VLA_IMGS_RIM_NIDX',
   'VLA_IMGS_THUMB_NEW',
   'VLA_IMGS_THUMB_NEW_NIDX',

# web pip
   'VLA_WEB_PLATINUM_JUPITER_BASE_PIP',
   'VLA_WEB_TIER0_JUPITER_BASE_PIP',
   'VLA_WEB_BASE_PIP_DEPLOY',

# imgs betas
   'VLA_IMGS_BASE_BETA',
   'VLA_IMGS_CBIR_BASE_BETA',
   'VLA_IMGS_TIER0_BASE',
   'VLA_IMGS_TIER0_CBIR_BASE',

# video pip
   'VLA_VIDEO_PLATINUM_BASE_PIP',
   'VLA_VIDEO_TIER0_BASE_PIP',

# web beta
   'VLA_VIDEO_PLATINUM_BASE_CONTENT_BETA',
   'VLA_VIDEO_TIER0_BASE_CONTENT_BETA',

# imgs pip
    'VLA_IMGS_BASE_PIP',
    'VLA_IMGS_CBIR_BASE_PIP',

# mmetas
    'VLA_MULTIMETA42_MMETA',
    'VLA_MULTIMETA44_MMETA',
]

computable = {
    'VLA_CALLISTO_DEPLOY',
    'VLA_PSI_LITE_ARNOLD',
}

optimized_followers = {
    'VLA_IMGS_BASE_MULTI',
    'VLA_IMGS_BASE_HAMSTER',
    'VLA_IMGS_BASE_NIDX',
    'VLA_IMGS_CBIR_BASE',
    'VLA_IMGS_CBIR_BASE_HAMSTER',
    'VLA_IMGS_CBIR_BASE_MULTI',
    'VLA_IMGS_CBIR_BASE_NIDX',

    'VLA_WEB_PLATINUM_JUPITER_BASE_HAMSTER',
    'VLA_WEB_PLATINUM_JUPITER_BASE_MULTI',
    'VLA_WEB_TIER0_JUPITER_BASE_HAMSTER',
    'VLA_WEB_TIER0_JUPITER_BASE_MULTI',

    'VLA_VIDEO_PLATINUM_BASE_MULTI',
    'VLA_VIDEO_TIER0_BASE_MULTI',
}

ints = [
    'VLA_WEB_CALLISTO_CAM_INT',
    'VLA_WEB_CALLISTO_CAM_INTL2',
    'VLA_WEB_CALLISTO_CAM_INT_HAMSTER',
    'VLA_WEB_CALLISTO_CAM_INTL2_HAMSTER',
    'VLA_WEB_TIER1_JUPITER_INT',
    'VLA_WEB_TIER1_JUPITER_INT_HAMSTER',
    'VLA_WEB_TIER1_JUPITER_INT_MULTI',
    'VLA_IMGS_INT',
    'VLA_IMGS_INT_BETA',
    'VLA_IMGS_INT_MULTI',
    'VLA_IMGS_INT_PIP',
    'VLA_IMGS_CBIR_INT',
    'VLA_IMGS_CBIR_INT_BETA',
    'VLA_IMGS_CBIR_INT_MULTI',
    'VLA_IMGS_CBIR_INT_PIP',
    'VLA_IMGS_TIER0_CBIR_INT',
    'VLA_IMGS_TIER0_INT',
    'VLA_WEB_PLATINUM_JUPITER_INT',
    'VLA_WEB_PLATINUM_JUPITER_INT_HAMSTER',
    'VLA_WEB_PLATINUM_JUPITER_INT_MULTI',
    'VLA_WEB_PLATINUM_JUPITER_INT_PIP',
    'VLA_WEB_TIER0_JUPITER_INT',
    'VLA_WEB_TIER0_JUPITER_INTL2_PIP',
    'VLA_WEB_TIER0_JUPITER_INT_HAMSTER',
    'VLA_WEB_TIER0_JUPITER_INT_MULTI',
    'VLA_WEB_TIER0_JUPITER_INT_PIP',
    'VLA_VIDEO_PLATINUM_INT',
    'VLA_VIDEO_PLATINUM_INT_MULTI',
    'VLA_VIDEO_PLATINUM_INT_PIP',
    'VLA_VIDEO_TIER0_INT',
    'VLA_VIDEO_TIER0_INT_MULTI',
    'VLA_VIDEO_TIER0_INT_PIP',
    'VLA_IMGS_T1_CBIR_INT',
    'VLA_IMGS_T1_INT',
    'VLA_WEB_INTL2',
    'VLA_WEB_INTL2_HAMSTER',
    'VLA_WEB_INTL2_MULTI',
]

to_remove = {
    'VLA_IMGS_TIER1_BASE',
    'VLA_IMGS_TIER1_CBIR_BASE',
    'VLA_IMGS_TIER1_CBIR_INT',
    'VLA_IMGS_TIER1_INT',
    'VLA_WEB_GEMINI_SEARCHPROXY_BALANCER_2',
    'VLA_WEB_GEMINI_SEARCHPROXY_DYN_2',
    'VLA_MULTIINT1_INTL2',
    'VLA_MULTIINT1_MMETA',
    'VLA_MULTIINT1_WEB_PLATINUM_INT',
    'VLA_MULTIINT1_WEB_TIER0_INT',
    'VLA_MULTIINT1_WEB_TIER1_INT',
    'VLA_IMGS_PROD_DEPLOY',
}

_all = {
    'non_optimized': non_optimized,
    'optimized_followers': optimized_followers,
    'ints': ints,
}


def dump(what):
    for group in _all[what]:
        print group


def test():
    print 'self-testing'
    total = set()
    for subset in (computable, to_remove, set(ints),
                   optimized, set(non_optimized), optimized_followers,
                   non_moveable.groups, non_moveable.mmetas, non_moveable.yt):
        assert total.isdisjoint(subset)
        total.update(subset)

    print 'no intersections'
    assert len(total) == 241
    print len(total), 'groups'
    print "compare with  cat ../../db/groups/VLA_YT_RTC/card.yaml | grep \"name\\\"\" | grep VLA | awk -F'\\\"' '{print $7}' | wc"
    print 'the expression above should give one group more, VLA_YT_RTC itself'


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--dump', help='dump given category', type=str)
    args = parser.parse_args()
    if args.dump:
        dump(args.dump)
    else:
        test()
