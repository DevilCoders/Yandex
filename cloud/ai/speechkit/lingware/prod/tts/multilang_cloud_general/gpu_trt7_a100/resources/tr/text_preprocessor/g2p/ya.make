OWNER(g:cloud-asr)

UNION(g2p)

FILES(
    TR.phone
    config
    hard_to_soft.map
    lts.map
    lts.re
    norm.re
    rule-based.config
    tr.affixes
    tr.auxilary_tags
    tr.dict
    tr.grapheme_groups
    tr.lts_phone_groups
    tr.prefix
    tr.roots
    tr.subst
    tr.suffix
    tr.valid_graphemes
    unvoiced_to_voiced.map
)

LARGE_FILES(
    flags.txt
    to_lower.map
)

END()
