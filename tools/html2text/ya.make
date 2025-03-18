PROGRAM()

OWNER(ibelotelov)

SRCS(
    html2text.cpp
    HtmlParser.cpp
    TextAndTitleNumerator.cpp
    TextAndTitleSentences.cpp
    SingleLineMapreduceFormatProcessor.cpp
)

PEERDIR(
    kernel/indexer/parseddoc
    library/cpp/getopt
)

END()
