package client

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

const jsonTopology = `{
    "revision": 1,
    "folder_id": "folder1",
    "service_account_id": "service_account_id1",
    "services": ["hdfs", "yarn", "mapreduce", "tez", "zookeeper", "hbase", "hive", "sqoop", "flume", "oozie", "livy"],
    "subcluster_main": {
        "cid": "cid1",
        "subcid": "subcid1",
        "name": "main",
        "services": ["hdfs", "yarn", "mapreduce", "zookeeper", "hbase", "hive", "sqoop", "oozie", "livy"],
        "resources": {
            "disk_size": 16106127360,
            "disk_type_id": "network-nvme",
            "resource_preset_id": "s1.compute.1"
        },
        "hosts": [
            "myt-1.df.cloud.yandex.net"
        ],
        "role": "hadoop_cluster.masternode",
        "subnet_id": "network1-myt",
        "assign_public_ip": false
    },
    "subclusters": [{
            "cid": "cid1",
            "subcid": "subcid2",
            "name": "data",
            "services": ["hdfs", "yarn", "mapreduce", "tez", "hbase", "flume"],
            "hosts_count": 5,
            "hosts": [
                "myt-2.df.cloud.yandex.net",
                "myt-3.df.cloud.yandex.net",
                "myt-4.df.cloud.yandex.net",
                "myt-5.df.cloud.yandex.net",
                "myt-6.df.cloud.yandex.net"
            ],
            "resources": {
                "disk_size": 16106127360,
                "disk_type_id": "network-nvme",
                "resource_preset_id": "s1.compute.1"
            },
            "role": "hadoop_cluster.datanode",
            "subnet_id": "network1-myt"
        }, {
            "cid": "cid1",
            "subcid": "subcid3",
            "name": "compute",
            "services": ["yarn", "mapreduce", "tez"],
            "hosts_count": 3,
            "hosts": [
                "myt-7.df.cloud.yandex.net",
                "myt-8.df.cloud.yandex.net",
                "myt-9.df.cloud.yandex.net"
            ],
            "resources": {
                "disk_size": 16106127360,
                "disk_type_id": "network-nvme",
                "resource_preset_id": "s1.compute.1"
            },
            "role": "hadoop_cluster.computenode",
            "subnet_id": "network1-myt"
        },
	    {
		  "cid": "e4uvej27ccc72jdcl7l4",
		  "name": "cid1",
		  "role": "hadoop_cluster.computenode",
          "instance_group_id": "bltima8d1edkcmrqqiop",
		  "hosts": [],
		  "subcid": "subcid4",
		  "services": [
			"yarn",
			"mapreduce",
			"tez"
		  ],
		  "resources": {
			"cores": 2,
			"memory": 8589934592,
			"disk_size": 21474836480,
			"disk_type_id": "network-ssd",
			"core_fraction": 100,
			"resource_preset_id": "s2.micro"
		  },
		  "subnet_id": "bltima8d1edkcmrqqiop",
		  "hosts_count": 1
		}
    ]
}`

func Test_parseClusterTopology(t *testing.T) {
	want := models.ClusterTopology{
		Cid:              "cid1",
		Revision:         1,
		FolderID:         "folder1",
		ServiceAccountID: "service_account_id1",
		Services:         []service.Service{service.Hdfs, service.Yarn, service.Zookeeper, service.Hbase, service.Hive, service.Oozie},
		Subclusters: []models.SubclusterTopology{
			{
				Subcid:   "subcid1",
				Role:     role.Main,
				Services: []service.Service{service.Hdfs, service.Yarn, service.Zookeeper, service.Hbase, service.Hive, service.Oozie},
				Hosts:    []string{"myt-1.df.cloud.yandex.net"},
			},
			{
				Subcid:   "subcid2",
				Role:     role.Data,
				Services: []service.Service{service.Hdfs, service.Yarn, service.Hbase},
				Hosts: []string{
					"myt-2.df.cloud.yandex.net",
					"myt-3.df.cloud.yandex.net",
					"myt-4.df.cloud.yandex.net",
					"myt-5.df.cloud.yandex.net",
					"myt-6.df.cloud.yandex.net",
				},
				MinHostsCount: 5,
			},
			{
				Subcid:   "subcid3",
				Role:     role.Compute,
				Services: []service.Service{service.Yarn},
				Hosts: []string{
					"myt-7.df.cloud.yandex.net",
					"myt-8.df.cloud.yandex.net",
					"myt-9.df.cloud.yandex.net",
				},
				MinHostsCount: 3,
			},
			{
				Subcid:          "subcid4",
				Role:            role.Compute,
				Services:        []service.Service{service.Yarn},
				Hosts:           []string{},
				InstanceGroupID: "bltima8d1edkcmrqqiop",
				MinHostsCount:   1,
			},
		},
	}

	got, err := parseClusterTopology("cid1", []byte(jsonTopology))
	require.NoError(t, err)
	require.Equal(t, want, got)
}
