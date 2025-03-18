#!/bin/bash -v 

BIN="$2"
SRCDIR="$3"
if [ "$1" == "val" ]; then
    EXE="valgrind ";
else
    BIN="$1"
    SRCDIR="$2"
fi
if [ -x "$BIN" ]; then
    EXE="${EXE}${BIN}"
else
    EXE="${EXE}./webxmltest"
fi
if [ ! -e "$SRCDIR" ]; then
    SRCDIR="."
fi

RECOGN="/var/lib/recogn.dict"
RSS_LINK="/home/trifon/work/s8/CORBA/controller/src/feedster/rss_link.cfg"

WORK="./test/work"
SRC="$SRCDIR/test/src"
QCR="$SRCDIR/test/qcr"

if [ ! -e ${WORK} ]; then
    mkdir -p ${WORK};
fi;

${EXE} digest > ${WORK}/gen.out;
${EXE} md5 "" >> ${WORK}/gen.out;
${EXE} md5 "a" >> ${WORK}/gen.out;
${EXE} md5 "abc" >> ${WORK}/gen.out;
${EXE} md5 "message digest" >> ${WORK}/gen.out;
${EXE} md5 "abcdefghijklmnopqrstuvwxyz" >> ${WORK}/gen.out;
${EXE} md5 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" >> ${WORK}/gen.out;
${EXE} md5 "12345678901234567890123456789012345678901234567890123456789012345678901234567890" >> ${WORK}/gen.out;

#doctor tests
${EXE} xmldoc ${SRC}/lesscode.org.xml ${WORK}/lesscode.org.xmldoc.out windows-1251;
${EXE} xmldoc ${SRC}/diary.xml ${WORK}/diary.xmldoc.out windows-1251;
${EXE} xmldoc ${SRC}/blogs.mail.ru.xml ${WORK}/blogs.mail.ru.xmldoc.out;
${EXE} xmldoc ${SRC}/webplanet.xml ${WORK}/webplanet.xmldoc.out;


${EXE} rss ${RECOGN} ${SRC}/russh.atom 20 > ${WORK}/russh.atom.out;
${EXE} rss ${RECOGN} ${SRC}/russh.rss > ${WORK}/russh.rss.out;
${EXE} rss ${RECOGN} ${SRC}/ebm.rss > ${WORK}/ebm.rss.out;
${EXE} rss ${RECOGN} ${SRC}/yoy.rss > ${WORK}/yoy.rss.out;
${EXE} rss ${RECOGN} ${SRC}/web.rss > ${WORK}/web.rss.out;
${EXE} rss ${RECOGN} ${SRC}/budilnik.rss > ${WORK}/budilnik.rss.out;
${EXE} rss ${RECOGN} ${SRC}/dist.rss > ${WORK}/dist.rss.out;
${EXE} rss ${RECOGN} ${SRC}/what.rss > ${WORK}/what.rss.out;
${EXE} rss ${RECOGN} ${SRC}/blogspot.atom > ${WORK}/blogspot.atom.out;
#files which need xmldoctor
${EXE} rss ${RECOGN} ${SRC}/lesscode.org.xml > ${WORK}/lesscode.org.xml.out;
${EXE} rss ${RECOGN} ${SRC}/diary.xml > ${WORK}/diary.xml.out;
${EXE} rss ${RECOGN} ${SRC}/blogs.mail.ru.xml > ${WORK}/blogs.mail.ru.xml.out;
${EXE} rss ${RECOGN} ${SRC}/webplanet.xml > ${WORK}/webplanet.xml.out;
${EXE} rss ${RECOGN} ${SRC}/kernel.rss > ${WORK}/kernel.rss.out;

${EXE} friends ${RECOGN} ${RSS_LINK} ${SRC}/foaf-8.xml > ${WORK}/foaf-8.xml.out
${EXE} friends ${RECOGN} ${RSS_LINK} ${SRC}/lenta.opml > ${WORK}/lenta.opml.out
${EXE} friends ${RECOGN} ${RSS_LINK} ${SRC}/opml.opml > ${WORK}/opml.opml.out
${EXE} friends ${RECOGN} ${RSS_LINK} ${SRC}/seealso.foaf > ${WORK}/seealso.foaf.out

DONE="OK";
for FF in gen russh.atom russh.rss ebm.rss yoy.rss web.rss budilnik.rss dist.rss what.rss blogspot.atom foaf-8.xml lenta.opml opml.opml seealso.foaf lesscode.org.xml diary.xml blogs.mail.ru.xml lesscode.org.xmldoc diary.xmldoc blogs.mail.ru.xmldoc webplanet.xml webplanet.xmldoc kernel.rss; do
    cmp ${WORK}/${FF}.out ${QCR}/${FF}.qcr >& /dev/null;
    if [ $? -ne 0 ]; then 
        echo "QCR FAIL: ${WORK}/${FF}.out ${QCR}/${FF}.qcr";
        DONE="With errors";
    else
        rm ${WORK}/${FF}.out;
    fi;
done;

if [ "${DONE}" == "OK" ]; then
    rm -rf ${WORK};
fi;

echo "DONE: ${DONE}"
