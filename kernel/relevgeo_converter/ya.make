LIBRARY()

OWNER(
    g:middle
    kulikov
    esoloviev
    vmordovin
)

ARCHIVE(
    NAME rules.inc
    all_regions.txt
    armenia.txt
    azerbaijan.txt
    belarus.txt
    countries.txt
    earth.txt
    estonia.txt
    georgia.txt
    images_countries.txt
    images_countries_ua.txt
    israel.txt
    kazakhstan.txt
    kyrgyzstan.txt
    lang_rules.txt
    lithuania.txt
    latvia.txt
    moldova.txt
    reg_russia.txt
    reg_ukraine.txt
    reg_turkey.txt
    rules.txt
    russia.txt
    tajikistan.txt
    turkey.txt
    turkmenistan.txt
    ukraine.txt
    upper_rules.txt
    usager.txt
    usa.txt
    uzbekistan.txt
    zero.txt
)

SRCS(
    converter.cpp
)

PEERDIR(
    kernel/groupattrs
    kernel/region2country
    kernel/search_types
    kernel/searchlog
    library/cpp/archive
    library/cpp/langs
)

END()
