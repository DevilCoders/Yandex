JAVA_LIBRARY(yandex-cloud-iam-fixtures)

JDK_VERSION(11)

OWNER(g:cloud-iam)

#JAVA_SRCS(SRCDIR src/main/resources **/*)

IF(YMAKE_JAVA_MODULES)
    FROM_SANDBOX(FILE 3280303075 RENAME yandex-cloud-iam-fixtures.jar OUT_NOAUTO sb.jar)
    JAVA_RESOURCE(sb.jar)
ELSE()
    EXTERNAL_JAR(sbr:3280303075)
ENDIF()

END()
