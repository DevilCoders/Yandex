package server

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/golang/protobuf/ptypes"
	"github.com/stretchr/testify/require"

	pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1"
	intapi "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/v1"
	ig "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/microcosm/instancegroup/v1"
	computemocks "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/compute-service/mocks"
	igmocks "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/instance-group-service/mocks"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/datastore"
	datastoremocks "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/datastore/mocks"
	intapimocks "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api/mocks"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/test/requirepb"
)

const (
	cid         = "cid1"
	cidNotFound = "not_found"
)

func TestServer_getClusterTopology(t *testing.T) {
	topologyNew := models.ClusterTopology{
		Cid:      cid,
		Revision: 2,
		FolderID: "folder1",
	}
	ctrl := gomock.NewController(t)
	iapi := intapimocks.NewMockInternalAPI(ctrl)
	iapi.EXPECT().GetClusterTopology(gomock.Any(), cid).Return(topologyNew, nil)
	iapi.EXPECT().GetClusterTopology(gomock.Any(), cidNotFound).Return(topologyNew, nil)

	topology := models.ClusterTopology{
		Cid:      cid,
		Revision: 1,
		FolderID: "folder1",
	}
	ds := datastoremocks.NewMockBackend(ctrl)
	ds.EXPECT().GetCachedClusterTopology(gomock.Any(), cid).Return(topology, nil).Times(2)
	ds.EXPECT().GetCachedClusterTopology(gomock.Any(), cidNotFound).
		Return(models.ClusterTopology{}, datastore.ErrNotFound)
	ds.EXPECT().StoreClusterTopology(gomock.Any(), cid, topologyNew).Return(nil)
	ds.EXPECT().StoreClusterTopology(gomock.Any(), cidNotFound, topologyNew).Return(nil)

	ctx := context.Background()
	serverRunner, err := NewServer(
		Config{},
		ds,
		iapi,
		&nop.Logger{},
		nil,
		nil,
	)
	require.NoError(t, err)

	got, err := serverRunner.getClusterTopology(ctx, cid, 1)
	require.NoError(t, err)
	require.Equal(t, topology, got)

	got, err = serverRunner.getClusterTopology(ctx, cid, 2)
	require.NoError(t, err)
	require.Equal(t, topologyNew, got)

	got, err = serverRunner.getClusterTopology(ctx, cidNotFound, 1)
	require.NoError(t, err)
	require.Equal(t, topologyNew, got)
}

func TestServer_Report(t *testing.T) {
	folder := "folder1"
	topology := models.ClusterTopology{
		Cid:              cid,
		Revision:         1,
		FolderID:         folder,
		Services:         []service.Service{service.Hdfs, service.Yarn},
		ServiceAccountID: "service_account_id1",
		Subclusters: []models.SubclusterTopology{
			{
				Subcid:   "data",
				Role:     role.Data,
				Services: []service.Service{service.Hdfs, service.Yarn},
				Hosts:    []string{"host-data-1", "host-data-2"},
			},
			{
				Subcid:   "compute",
				Role:     role.Compute,
				Services: []service.Service{service.Yarn},
				Hosts:    []string{"host-compute-1", "host-compute-2"},
			},
			{
				Subcid:          "compute_autoscaling",
				Role:            role.Compute,
				Services:        []service.Service{service.Yarn},
				Hosts:           []string{},
				MinHostsCount:   1,
				InstanceGroupID: "instance_group_id_1",
			},
		},
	}
	ctrl := gomock.NewController(t)
	ds := datastoremocks.NewMockBackend(ctrl)
	igcomputemock := igmocks.NewMockInstanceGroupServiceClient(ctrl)
	computemock := computemocks.NewMockComputeServiceClient(ctrl)

	ds.EXPECT().GetCachedClusterTopology(gomock.Any(), cid).Return(topology, nil)

	clusterHealth := models.ClusterHealth{
		Cid:    cid,
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Hdfs: models.ServiceHdfs{
				BasicHealthService: models.BasicHealthService{Health: health.Alive},
			},
			service.Yarn: models.BasicHealthService{Health: health.Alive},
		},
	}
	ds.EXPECT().StoreClusterHealth(gomock.Any(), cid, clusterHealth).Return(nil)

	hostsHealth := map[string]models.HostHealth{
		"host-data-1": {
			Fqdn:   "host-data-1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hdfs: models.ServiceHdfsNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					State:              "In Service",
				},
				service.Yarn: models.ServiceYarnNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					State:              "RUNNING",
				},
			},
		},
		"host-data-2": {
			Fqdn:   "host-data-2",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hdfs: models.ServiceHdfsNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					State:              "In Service",
				},
				service.Yarn: models.ServiceYarnNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					State:              "RUNNING",
				},
			},
		},
		"host-compute-1": {
			Fqdn:   "host-compute-1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Yarn: models.ServiceYarnNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					State:              "RUNNING",
				},
			},
		},
		"host-compute-2": {
			Fqdn:   "host-compute-2",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Yarn: models.ServiceYarnNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					State:              "RUNNING",
				},
			},
		},
		"rc1b-dataproc-g-ybef.mdb.cloud-preprod.yandex.net": {
			Fqdn:   "rc1b-dataproc-g-ybef.mdb.cloud-preprod.yandex.net",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Yarn: models.ServiceYarnNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					State:              "RUNNING",
				},
			},
		},
	}

	ds.EXPECT().StoreHostsHealth(gomock.Any(), cid, hostsHealth).Return(nil)
	igcomputemock.EXPECT().Get(gomock.Any(), gomock.Any(), gomock.Any()).Return(&ig.InstanceGroup{}, nil)

	ds.EXPECT().LoadDecommissionHosts(gomock.Any(), cid).Return(models.DecommissionHosts{}, nil)
	ds.EXPECT().DeleteDecommissionHosts(gomock.Any(), cid).Return(nil)

	ds.EXPECT().StoreDecommissionStatus(gomock.Any(), cid, models.DecommissionStatus{}).Return(nil)

	iapi := intapimocks.NewMockInternalAPI(ctrl)
	ctx := context.Background()
	ctx = context.WithValue(ctx, folderIDKey, folder)
	serverRunner, err := NewServer(
		Config{NoAuth: true},
		ds,
		iapi,
		&nop.Logger{},
		computemock,
		igcomputemock,
	)
	require.NoError(t, err)

	report := pb.ReportRequest{
		Cid:              cid,
		TopologyRevision: 1,
		Info: &pb.Info{
			Hdfs: &pb.HDFSInfo{
				Available: true,
				LiveNodes: []*pb.HDFSNodeInfo{
					{Name: "host-data-1", State: "In Service"},
					{Name: "host-data-2", State: "In Service"},
				},
			},
			Yarn: &pb.YarnInfo{
				Available: true,
				LiveNodes: []*pb.YarnNodeInfo{
					{Name: "host-compute-1", State: "RUNNING"},
					{Name: "host-compute-2", State: "RUNNING"},
					{Name: "host-data-1", State: "RUNNING"},
					{Name: "host-data-2", State: "RUNNING"},
					{Name: "rc1b-dataproc-g-ybef.mdb.cloud-preprod.yandex.net", State: "RUNNING"},
				},
			},
		},
	}

	got, err := serverRunner.Report(ctx, &report)
	require.NoError(t, err)
	requirepb.Equal(
		t,
		&pb.ReportReply{
			DecommissionTimeout:     0,
			YarnHostsToDecommission: nil,
			HdfsHostsToDecommission: nil,
		},
		got,
	)
}

func TestServer_ClusterHealth(t *testing.T) {
	ctrl := gomock.NewController(t)
	iapi := intapimocks.NewMockInternalAPI(ctrl)
	ds := datastoremocks.NewMockBackend(ctrl)
	clusterHealth := models.ClusterHealth{
		Cid:    cid,
		Health: health.Degraded,
		Services: map[service.Service]models.ServiceHealth{
			service.Hdfs:      models.BasicHealthService{Health: health.Alive},
			service.Hive:      models.BasicHealthService{Health: health.Degraded},
			service.Mapreduce: models.BasicHealthService{Health: health.Alive},
			service.Yarn:      models.BasicHealthService{Health: health.Alive},
			service.Tez:       models.BasicHealthService{Health: health.Dead},
		},
	}
	ds.EXPECT().LoadClusterHealth(gomock.Any(), cid).Return(clusterHealth, nil).Times(1)
	ds.EXPECT().LoadClusterHealth(gomock.Any(), cidNotFound).Return(models.ClusterHealth{}, datastore.ErrNotFound).Times(1)

	ctx := context.Background()
	serverRunner, err := NewServer(
		Config{},
		ds,
		iapi,
		&nop.Logger{},
		nil,
		nil,
	)
	require.NoError(t, err)

	got, err := serverRunner.ClusterHealth(ctx, &pb.ClusterHealthRequest{Cid: cid})
	want := &pb.ClusterHealthReply{
		Cid:    cid,
		Health: pb.Health_DEGRADED,
		ServiceHealth: []*pb.ServiceHealth{
			{
				Service: pb.Service_HDFS,
				Health:  pb.Health_ALIVE,
			},
			{
				Service: pb.Service_HIVE,
				Health:  pb.Health_DEGRADED,
			},
			{
				Service: pb.Service_MAPREDUCE,
				Health:  pb.Health_ALIVE,
			},
			{
				Service: pb.Service_YARN,
				Health:  pb.Health_ALIVE,
			},
			{
				Service: pb.Service_TEZ,
				Health:  pb.Health_DEAD,
			},
		},
	}
	require.NoError(t, err)
	require.ElementsMatch(t, want.ServiceHealth, got.ServiceHealth)
	want.ServiceHealth = []*pb.ServiceHealth{}
	got.ServiceHealth = []*pb.ServiceHealth{}
	requirepb.Equal(t, want, got)

	_, err = serverRunner.ClusterHealth(ctx, &pb.ClusterHealthRequest{Cid: cidNotFound})
	require.Error(t, err)
}

func TestServer_HostsHealth(t *testing.T) {
	ctrl := gomock.NewController(t)
	iapi := intapimocks.NewMockInternalAPI(ctrl)
	ds := datastoremocks.NewMockBackend(ctrl)

	hostsHealth := map[string]models.HostHealth{
		"host1": {
			Fqdn:   "host1",
			Health: health.Alive,
		},
	}
	ds.EXPECT().LoadHostsHealth(gomock.Any(), cid, []string{"host1"}).Return(hostsHealth, nil)

	unknownHostHealth := map[string]models.HostHealth{
		"host1": {
			Fqdn:   "host1",
			Health: health.Unknown,
		},
	}
	ds.EXPECT().LoadHostsHealth(gomock.Any(), cidNotFound, []string{"host1"}).Return(unknownHostHealth, nil)

	ctx := context.Background()
	serverRunner, err := NewServer(
		Config{},
		ds,
		iapi,
		&nop.Logger{},
		nil,
		nil,
	)
	require.NoError(t, err)

	request := &pb.HostsHealthRequest{
		Cid:   cid,
		Fqdns: []string{"host1"},
	}
	got, err := serverRunner.HostsHealth(ctx, request)
	require.NoError(t, err)

	//Using map for readable error
	gotHealth := make(map[string]*pb.HostHealth, len(got.HostsHealth))
	for _, hh := range got.HostsHealth {
		gotHealth[hh.Fqdn] = hh
	}

	want := map[string]*pb.HostHealth{
		"host1": &pb.HostHealth{
			Fqdn:          "host1",
			Health:        pb.Health_ALIVE,
			ServiceHealth: make([]*pb.ServiceHealth, 0),
		},
	}
	require.NoError(t, err)
	requirepb.Equal(t, want, gotHealth)

	request.Cid = cidNotFound
	got, err = serverRunner.HostsHealth(ctx, request)
	require.NoError(t, err)
	require.Len(t, got.HostsHealth, 1)
	wantHost := &pb.HostHealth{
		Fqdn:          "host1",
		Health:        pb.Health_HEALTH_UNSPECIFIED,
		ServiceHealth: make([]*pb.ServiceHealth, 0),
	}
	requirepb.Equal(t, got.HostsHealth[0], wantHost)
}

func TestServer_UpdateStatus(t *testing.T) {
	ctrl := gomock.NewController(t)
	intapiMock := intapimocks.NewMockInternalAPI(ctrl)

	intapiMock.EXPECT().UpdateJobStatus(gomock.Any(), &intapi.UpdateJobStatusRequest{
		ClusterId: "777",
		JobId:     "666",
		Status:    intapi.Job_RUNNING,
	}).Return(&intapi.UpdateJobStatusResponse{
		ClusterId: "777",
		JobId:     "666",
		Status:    intapi.Job_RUNNING,
	}, nil)

	serverRunner, err := NewServer(
		Config{NoAuth: true},
		nil,
		intapiMock,
		&nop.Logger{},
		nil,
		nil,
	)
	require.NoError(t, err)

	request := &pb.UpdateJobStatusRequest{
		ClusterId: "777",
		JobId:     "666",
		Status:    pb.Job_RUNNING,
	}
	_, err = serverRunner.UpdateStatus(context.Background(), request)
	require.NoError(t, err)
}

func TestServer_ListActive(t *testing.T) {
	ctrl := gomock.NewController(t)
	intapiMock := intapimocks.NewMockInternalAPI(ctrl)
	createdAt := ptypes.TimestampNow()
	startedAt := ptypes.TimestampNow()

	intapiMock.EXPECT().ListClusterJobs(gomock.Any(), &intapi.ListJobsRequest{
		ClusterId: "777",
		PageSize:  9,
		PageToken: "token1",
		Filter:    "status='active'",
	}).Return(&intapi.ListJobsResponse{
		Jobs: []*intapi.Job{
			{
				Id:        "001",
				ClusterId: "777",
				Name:      "job 001",
				Status:    intapi.Job_RUNNING,
				CreatedAt: createdAt,
				StartedAt: startedAt,
				JobSpec:   &intapi.Job_SparkJob{SparkJob: &intapi.SparkJob{}},
			},
			{
				JobSpec: &intapi.Job_SparkJob{SparkJob: &intapi.SparkJob{
					Args:           []string{"--driver-cores", "2", "--executor-memory", "2G"},
					JarFileUris:    []string{"file1.jar", "file2.jar"},
					FileUris:       []string{"file1.txt", "file2.txt"},
					ArchiveUris:    []string{"archive1.zip", "archive2.zip"},
					Properties:     map[string]string{"key1": "val1", "key2": "val2"},
					MainJarFileUri: "s3a://bucket/WordCount.jar",
					MainClass:      "SpecialWordCount",
					Packages: []string{"org.apache.spark:spark-streaming-kafka_2.10:1.6.0",
						"org.elasticsearch:elasticsearch-spark_2.10:2.2.0"},
					Repositories: []string{"https://oss.sonatype.org/content/groups/public/",
						"https://oss.sonatype.org/content/groups/public/"},
					ExcludePackages: []string{"org.apache.spark:spark-streaming-kafka_2.10"},
				}},
			},
			{
				JobSpec: &intapi.Job_PysparkJob{PysparkJob: &intapi.PysparkJob{
					Args:              []string{"--driver-cores", "2", "--executor-memory", "2G"},
					ArchiveUris:       []string{"archive1.zip", "archive2.zip"},
					FileUris:          []string{"file1.txt", "file2.txt"},
					JarFileUris:       []string{"file1.jar", "file2.jar"},
					PythonFileUris:    []string{"file1.py", "file2.py"},
					Properties:        map[string]string{"key1": "val1", "key2": "val2"},
					MainPythonFileUri: "main.py",
					Packages: []string{"org.apache.spark:spark-streaming-kafka_2.10:1.6.0",
						"org.elasticsearch:elasticsearch-spark_2.10:2.2.0"},
					Repositories: []string{"https://oss.sonatype.org/content/groups/public/",
						"https://oss.sonatype.org/content/groups/public/"},
					ExcludePackages: []string{"org.apache.spark:spark-streaming-kafka_2.10"},
				}},
			},
			{
				JobSpec: &intapi.Job_MapreduceJob{MapreduceJob: &intapi.MapreduceJob{
					Args:        []string{"-mapper", "mapper.py", "-reducer", "reducer.py"},
					ArchiveUris: []string{"archive1.zip", "archive2.zip"},
					FileUris:    []string{"s3a://bucket/mapper.py", "s3a://bucket/reducer.py"},
					JarFileUris: []string{"file1.jar", "file2.jar"},
					Properties:  map[string]string{"key1": "val1", "key2": "val2"},
					Driver:      &intapi.MapreduceJob_MainJarFileUri{MainJarFileUri: "hadoop-streaming.jar"},
				}},
			},
			{
				JobSpec: &intapi.Job_MapreduceJob{MapreduceJob: &intapi.MapreduceJob{
					Driver: &intapi.MapreduceJob_MainClass{MainClass: "WordCount"},
				}},
			},
			{
				JobSpec: &intapi.Job_HiveJob{HiveJob: &intapi.HiveJob{
					ContinueOnFailure: true,
					ScriptVariables:   map[string]string{"var1": "val1", "var2": "val2"},
					Properties:        map[string]string{"prop1": "val3", "prop2": "val4"},
					JarFileUris:       []string{"file1.jar", "file2.jar"},
					QueryType:         &intapi.HiveJob_QueryFileUri{QueryFileUri: "script.sql"},
				}},
			},
			{
				JobSpec: &intapi.Job_HiveJob{HiveJob: &intapi.HiveJob{
					QueryType: &intapi.HiveJob_QueryList{QueryList: &intapi.QueryList{
						Queries: []string{"query1", "query2"},
					}},
				}},
			},
		},
		NextPageToken: "token2",
	}, nil)

	serverRunner, err := NewServer(
		Config{NoAuth: true},
		nil,
		intapiMock,
		&nop.Logger{},
		nil,
		nil,
	)
	require.NoError(t, err)

	request := &pb.ListJobsRequest{
		ClusterId: "777",
		PageSize:  9,
		PageToken: "token1",
		Filter:    "fff",
	}
	response, err := serverRunner.ListActive(context.Background(), request)
	require.NoError(t, err)
	require.Equal(t, "token2", response.NextPageToken)
	requirepb.Equal(t, []*pb.Job{
		{
			Id:        "001",
			ClusterId: "777",
			Name:      "job 001",
			Status:    pb.Job_RUNNING,
			CreatedAt: createdAt,
			StartedAt: startedAt,
			JobSpec:   &pb.Job_SparkJob{SparkJob: &pb.SparkJob{}},
		},
		{
			JobSpec: &pb.Job_SparkJob{SparkJob: &pb.SparkJob{
				Args:           []string{"--driver-cores", "2", "--executor-memory", "2G"},
				JarFileUris:    []string{"file1.jar", "file2.jar"},
				FileUris:       []string{"file1.txt", "file2.txt"},
				ArchiveUris:    []string{"archive1.zip", "archive2.zip"},
				Properties:     map[string]string{"key1": "val1", "key2": "val2"},
				MainJarFileUri: "s3a://bucket/WordCount.jar",
				MainClass:      "SpecialWordCount",
				Packages: []string{"org.apache.spark:spark-streaming-kafka_2.10:1.6.0",
					"org.elasticsearch:elasticsearch-spark_2.10:2.2.0"},
				Repositories: []string{"https://oss.sonatype.org/content/groups/public/",
					"https://oss.sonatype.org/content/groups/public/"},
				ExcludePackages: []string{"org.apache.spark:spark-streaming-kafka_2.10"},
			}},
		},
		{
			JobSpec: &pb.Job_PysparkJob{PysparkJob: &pb.PysparkJob{
				Args:              []string{"--driver-cores", "2", "--executor-memory", "2G"},
				ArchiveUris:       []string{"archive1.zip", "archive2.zip"},
				FileUris:          []string{"file1.txt", "file2.txt"},
				JarFileUris:       []string{"file1.jar", "file2.jar"},
				PythonFileUris:    []string{"file1.py", "file2.py"},
				Properties:        map[string]string{"key1": "val1", "key2": "val2"},
				MainPythonFileUri: "main.py",
				Packages: []string{"org.apache.spark:spark-streaming-kafka_2.10:1.6.0",
					"org.elasticsearch:elasticsearch-spark_2.10:2.2.0"},
				Repositories: []string{"https://oss.sonatype.org/content/groups/public/",
					"https://oss.sonatype.org/content/groups/public/"},
				ExcludePackages: []string{"org.apache.spark:spark-streaming-kafka_2.10"},
			}},
		},
		{
			JobSpec: &pb.Job_MapreduceJob{MapreduceJob: &pb.MapreduceJob{
				Args:        []string{"-mapper", "mapper.py", "-reducer", "reducer.py"},
				ArchiveUris: []string{"archive1.zip", "archive2.zip"},
				FileUris:    []string{"s3a://bucket/mapper.py", "s3a://bucket/reducer.py"},
				JarFileUris: []string{"file1.jar", "file2.jar"},
				Properties:  map[string]string{"key1": "val1", "key2": "val2"},
				Driver:      &pb.MapreduceJob_MainJarFileUri{MainJarFileUri: "hadoop-streaming.jar"},
			}},
		},
		{
			JobSpec: &pb.Job_MapreduceJob{MapreduceJob: &pb.MapreduceJob{
				Driver: &pb.MapreduceJob_MainClass{MainClass: "WordCount"},
			}},
		},
		{
			JobSpec: &pb.Job_HiveJob{HiveJob: &pb.HiveJob{
				ContinueOnFailure: true,
				ScriptVariables:   map[string]string{"var1": "val1", "var2": "val2"},
				Properties:        map[string]string{"prop1": "val3", "prop2": "val4"},
				JarFileUris:       []string{"file1.jar", "file2.jar"},
				QueryType:         &pb.HiveJob_QueryFileUri{QueryFileUri: "script.sql"},
			}},
		},
		{
			JobSpec: &pb.Job_HiveJob{HiveJob: &pb.HiveJob{
				QueryType: &pb.HiveJob_QueryList{QueryList: &pb.QueryList{
					Queries: []string{"query1", "query2"},
				}},
			}},
		},
	}, response.Jobs)
}
