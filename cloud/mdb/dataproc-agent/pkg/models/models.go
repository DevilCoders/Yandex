package models

// HbaseNodeInfo holds Hbase node info
type HbaseNodeInfo struct {
	Name          string `json:"name"`
	Requests      int64  `json:"requests"`
	HeapSizeMB    int64  `json:"heapSizeMB"`
	MaxHeapSizeMB int64  `json:"maxHeapSizeMB"`
}

// HbaseInfo holds Hbase cluster info
type HbaseInfo struct {
	Available   bool    `json:"-"`
	Regions     int64   `json:"regions"`
	Requests    int64   `json:"requests"`
	AverageLoad float64 `json:"averageLoad"`
	LiveNodes   []HbaseNodeInfo
	DeadNodes   []HbaseNodeInfo
}

// HDFSNodeInfo holds HDFS datanode status info
type HDFSNodeInfo struct {
	Name      string `json:"-"`
	Used      int64  `json:"used"`
	Remaining int64  `json:"remaining"`
	Capacity  int64  `json:"capacity"`
	NumBlocks int64  `json:"numBlocks"`
	State     string `json:"adminState"`
}

// HDFSInfo holds HDFS cluster status info
type HDFSInfo struct {
	Available                                     bool `json:"-"`
	PercentRemaining                              float64
	Used                                          int64
	Free                                          int64
	TotalBlocks                                   int64
	NumberOfMissingBlocks                         int64
	NumberOfMissingBlocksWithReplicationFactorOne int64
	LiveNodes                                     []HDFSNodeInfo `json:"-"`
	DeadNodes                                     []HDFSNodeInfo `json:"-"`
	DecommissioningNodes                          []HDFSNodeInfo `json:"-"`
	DecommissionedNodes                           []HDFSNodeInfo `json:"-"`
	Safemode                                      string
	RequestedDecommissionHosts                    []string // Actual list of decommission hosts in HDFS namenode memory
}

// HiveInfo holds Hive status info
type HiveInfo struct {
	Available        bool `json:"-"`
	QueriesSucceeded int64
	QueriesFailed    int64
	QueriesExecuting int64
	SessionsOpen     int64
	SessionsActive   int64
}

// YARNInfo holds YARN cluster status info
type YARNInfo struct {
	Available                  bool
	LiveNodes                  []YARNNodeInfo
	RequestedDecommissionHosts []string // Actual list of decommission hosts in YARN ResourceManager memory
}

type YARNNodeInfo struct {
	Name              string `json:"nodeHostName"`
	State             string `json:"state"`
	NumContainers     int64  `json:"numContainers"`
	UsedMemoryMB      int64  `json:"usedMemoryMB"`
	AvailableMemoryMB int64  `json:"availMemoryMB"`
	UpdateTime        int64  `json:"lastHealthUpdate"`
}

// ZookeeperInfo holds Zookeeper cluster status info
type ZookeeperInfo struct {
	Available bool
}

// OozieInfo holds Oozie status info
type OozieInfo struct {
	Available bool
}

// LivyInfo holds Apache Livy status info
type LivyInfo struct {
	Available bool
}

// Info describes cluster health info
type Info struct {
	Cid              string        `json:"cid"`
	TopologyRevision int64         `json:"topology_revision"`
	ReportCount      int64         `json:"report_count"`
	HBase            HbaseInfo     `json:"hbase"`
	HDFS             HDFSInfo      `json:"hdfs"`
	Hive             HiveInfo      `json:"hive"`
	YARN             YARNInfo      `json:"yarn"`
	Zookeeper        ZookeeperInfo `json:"zk"`
	Oozie            OozieInfo     `json:"oozie"`
	Livy             LivyInfo      `json:"livy"`
}

// NodesToDecommission describes list of cluster nodes to be decommissioned
type NodesToDecommission struct {
	DecommissionTimeout     int
	YarnHostsToDecommission []string
	HdfsHostsToDecommission []string
}
