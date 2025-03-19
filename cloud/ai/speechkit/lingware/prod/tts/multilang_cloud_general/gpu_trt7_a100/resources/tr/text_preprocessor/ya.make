OWNER(g:cloud-asr)

UNION()

FILES(
    label.conf
    punc.conf
    subst.conf
)

END()

RECURSE(
    g2p
    norm_restorer
    postagger
)
