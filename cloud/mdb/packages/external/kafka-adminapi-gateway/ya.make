OWNER(g:mdb-dataproc)

JAVA_PROGRAM()

JDK_VERSION(11)

JAVA_SRCS(
    src/main/java **/*
)

PEERDIR(
	contrib/java/net/sf/py4j/py4j/0.10.9.3
	contrib/java/org/apache/kafka/kafka-clients/2.7.2
	contrib/java/org/apache/kafka/kafka_2.13/2.7.2
)

UBERJAR()

UBERJAR_MANIFEST_TRANSFORMER_MAIN(KafkaAdmin)

END()
