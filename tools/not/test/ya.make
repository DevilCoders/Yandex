OWNER(melkov g:ymake)

EXECTEST()

RUN(
    NAME "not quietly returns 1 without arguments"
    not not
)
RUN(
    NAME "not returns error when tryin to run non-existent program"
    not not really-non-existent-file.txt
)

DEPENDS(tools/not)

END()
