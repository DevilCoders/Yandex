UNITTEST_FOR(library/cpp/on_disk/tar_archive)
OWNER(
    alexmir0x1
)
SIZE(MEDIUM)
DATA(
    sbr://1609595113 # test_data/
    sbr://1609581427 # test_data.tar
)
SRCS(
    archive_iterator_ut.cpp
    archive_writer_ut.cpp
)
END()
