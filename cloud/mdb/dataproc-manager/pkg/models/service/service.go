package service

// Service is service type enum
type Service int

const (
	// Hdfs service
	Hdfs Service = iota + 1
	// Yarn service
	Yarn
	// Mapreduce service
	Mapreduce
	// Hive service
	Hive
	// Tez service
	Tez
	// Zookeeper service
	Zookeeper
	// Hbase service
	Hbase
	// Sqoop service
	Sqoop
	// Flume service
	Flume
	// Spark service
	Spark
	// Zeppelin service
	Zeppelin
	// Apache Oozie service
	Oozie
	// Apache Livy
	Livy
)

func (service Service) String() string {
	names := [...]string{
		"HDFS",
		"YARN",
		"MapReduce",
		"Hive",
		"Tez",
		"Zookeeper",
		"HBase",
		"Sqoop",
		"Flume",
		"Spark",
		"Zeppelin",
		"Oozie",
		"Livy"}
	return names[service-1]
}
