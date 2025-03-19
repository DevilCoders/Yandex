OWNER(g:cloud-asr)

UNION(g2p)

FILES(
    EN.phone
    config
    een.dict.compressed
    en.auxilary_tags
    en.dict
    en.grapheme_groups
    en.grapheme_onsets
    en.lts_phone_groups
    en.misc_rulebased
    en.onsets
    en.prefix
    en.reduced_vowels
    en.sonor_cons
    en.stressed_vowels
    en.subst
    en.subst.compressed
    en.suffix
    en.valid_graphemes
    en.vowels
    example.txt
    flags.txt
    hard_to_soft.map
    lts.map
    lts.map.gr
    lts.re
    norm.re
    pos.dict
    pos.dict.compressed
    rule-based.config
    to_lower.map
    unvoiced_to_voiced.map
)

LARGE_FILES(
    fancy/model
    fancy/model.nnet
)

END()
