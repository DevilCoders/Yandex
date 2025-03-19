JAVA_PROGRAM()

JDK_VERSION(11)

OWNER(g:cloud-kms)

UBERJAR()

WITH_KOTLIN()

PEERDIR(
	contrib/java/com/amazonaws/aws-java-sdk-s3/1.11.792
	contrib/java/commons-cli/commons-cli/1.4
	contrib/java/commons-io/commons-io/2.8.0
)

JAVA_SRCS(SRCDIR src/main/kotlin **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

CHECK_JAVA_DEPS(yes)

END()
