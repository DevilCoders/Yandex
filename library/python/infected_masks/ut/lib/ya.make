OWNER(
    g:antimalware
    g:antiwebspam
)

TEST_SRCS(
    masks_comptrie_test.py
)

SRCDIR(
    library/python/infected_masks/ut/lib
)

PEERDIR(
    library/python/infected_masks
)

DATA(
    sbr://725944867=fixtures/test_trie
)
