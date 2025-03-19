package intapi

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"strings"
	"text/template"

	"github.com/go-resty/resty/v2"
	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"github.com/jhump/protoreflect/dynamic"
	"github.com/jhump/protoreflect/dynamic/grpcdynamic"
	"github.com/jhump/protoreflect/grpcreflect"
	"google.golang.org/grpc"
	reflectpb "google.golang.org/grpc/reflection/grpc_reflection_v1alpha"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	client  *resty.Client
	baseURL string
}

func restURIBase() string {
	port, ok := os.LookupEnv("DBAAS_INTERNAL_API_RECIPE_PORT")
	if !ok {
		panic("dbaas internal api port is not set")
	}

	return "http://localhost:" + port
}

func New() *Client {
	return &Client{
		client:  resty.New(),
		baseURL: restURIBase(),
	}
}

type CreateClusterRequest struct {
	Name     string
	DiskSize int64
}

var (
	defaultClusterDiskSize int64 = 10737418240

	clusterDiskSize = map[string]int64{
		"hadoop": 16106127360,
		"redis":  17179869184,
	}
)

var createClusterTemplates = map[string]*template.Template{
	"postgresql": createPGClusterTmpl,
	"mysql":      createMyClusterTmpl,
	"mongodb":    createMongoDBClusterTmpl,
	"clickhouse": createClickHouseClusterTmpl,
	"hadoop":     createHadoopClusterRequest,
	"redis":      createRedisClusterTmpl,
}

var createClusterTemplatesGoAPI = map[string]*template.Template{
	"greenplum":     createGreenplumClusterTmpl,
	"elasticsearch": createElasticsearchClusterTmpl,
}

func templateMustFromString(text string) *template.Template {
	return template.Must(template.New("").Parse(text))
}

type CreatePostgresqlClusterRequest struct {
	Name             string
	Description      string
	FolderID         string
	Token            string
	ResourcePresetID string
	DiskTypeID       string
	NetworkID        string
	Zones            []string
	Environment      string
}

var createPGClusterTmpl = templateMustFromString(`
{
   "name": "{{ .Name }}",
   "environment": "PRESTABLE",
   "configSpec": {
	   "version": "14",
	   "postgresqlConfig_14": {},
	   "resources": {
		   "resourcePresetId": "s1.porto.1",
		   "diskTypeId": "local-ssd",
		   "diskSize": {{ .DiskSize }}
	   }
   },
   "databaseSpecs": [{
	   "name": "testdb",
	   "owner": "test"
   }],
   "userSpecs": [{
	   "name": "test",
	   "password": "test_password"
   }],
	"hostSpecs": [{
		"zoneId": "myt"
	}, {
		"zoneId": "iva"
	}, {
		"zoneId": "sas"
	}],
   "description": "test cluster"
}
`)

var createPGClusterExtendedTmpl = templateMustFromString(`
{
   "name": "{{ .Name }}",
   "environment": "{{ .Environment }}",
   "configSpec": {
	   "version": "14",
	   "postgresqlConfig_14": {},
	   "resources": {
		   "resourcePresetId": "{{ .ResourcePresetID }}",
		   "diskTypeId": "{{ .DiskTypeID }}",
		   "diskSize": 10737418240
	   }
   },
   "databaseSpecs": [{
	   "name": "testdb",
	   "owner": "test"
   }],
   "userSpecs": [{
	   "name": "test",
	   "password": "test_password"
   }],
   "hostSpecs": [
	{{ range $i, $zone := .Zones }}
		{{ if $i }}, {{ end }}{ "zoneId": "{{ $zone }}" }
	{{ end }} ],
   "description": "{{ .Description }}",
   "networkId": "{{ .NetworkID }}"
}
`)

var createMyClusterTmpl = templateMustFromString(`
{
	"name": "{{ .Name }}",
	"environment": "PRESTABLE",
	"configSpec": {
		"mysqlConfig_5_7": {
		},
		"resources": {
			"resourcePresetId": "s1.porto.1",
			"diskTypeId": "local-ssd",
		    "diskSize": {{ .DiskSize }}
		}
	},
	"databaseSpecs": [{
		"name": "testdb"
	}],
	"userSpecs": [{
		"name": "test",
		"password": "test_password"
	}],
	"hostSpecs": [{
		"zoneId": "myt"
	}, {
		"zoneId": "iva"
	}, {
		"zoneId": "sas"
	}],
	"description": "test cluster",
	"networkId": "IN-PORTO-NO-NETWORK-API"
}
`)

var createHadoopClusterRequest = templateMustFromString(`
{
	"folderId": "folder1",
	"name": "{{ .Name }}",
	"configSpec": {
		"hadoop": {
			"sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"]
		},
		"subclustersSpec": [
			{
				"name": "main",
				"role": "MASTERNODE",
				"resources": {
					"resourcePresetId": "s1.compute.1",
					"diskTypeId": "network-ssd",
     				"diskSize": {{ .DiskSize }}
				},
				"subnetId": "network1-myt"
			},
			{
				"name": "data",
				"role": "DATANODE",
				"resources": {
					"resourcePresetId": "s1.compute.1",
					"diskTypeId": "network-ssd",
				    "diskSize": {{ .DiskSize }}
				},
				"hostsCount": 5,
				"subnetId": "network1-myt"
			}
		]
	},
	"description": "test cluster",
	"zoneId": "myt",
	"serviceAccountId": "service_account_1",
	"bucket": "user_s3_bucket"
}
`)

var createHostJs = `{
	"hostSpecs": [{
		"zoneId": "man"
	}]
}`

type CreateClickHouseClusterRequest struct {
	Name             string
	ResourcePresetID string
	DiskTypeID       string
	Zones            []string
	NetworkID        string
}

var createClickHouseClusterTmpl = templateMustFromString(`
{
	"name": "{{ .Name }}",
	"environment": "PRESTABLE",
	"configSpec": {
		"clickhouse": {
			"resources": {
				"resourcePresetId": "s1.porto.1",
				"diskTypeId": "local-ssd",
				"diskSize": {{ .DiskSize }}
			}
		}
	},
	"databaseSpecs": [{
		"name": "testdb"
	}],
	"userSpecs": [{
		"name": "test",
		"password": "test_password"
	}],
	"hostSpecs": [{
		"type": "CLICKHOUSE",
		"zoneId": "man"
	}],
	"description": "test cluster"
}
`)

var createClickHouseClusterExtendedTmpl = templateMustFromString(`
{
	"name": "{{ .Name }}",
	"environment": "PRESTABLE",
	"configSpec": {
		"clickhouse": {
			"resources": {
				"resourcePresetId": "{{ .ResourcePresetID }}",
				"diskTypeId": "{{ .DiskTypeID }}",
				"diskSize": 10737418240
			}
		}
	},
	"databaseSpecs": [{
		"name": "testdb"
	}],
	"userSpecs": [{
		"name": "test",
		"password": "test_password"
	}],
	"hostSpecs": [
	{{ range $i, $zone := .Zones }}
		{{ if $i }}, {{ end }}{ "type": "CLICKHOUSE", "zoneId": "{{ $zone }}" }
	{{ end }} ],
	"networkId": "{{ .NetworkID }}",
	"description": "test cluster"
}
`)

type CreateMongoDBClusterRequest struct {
	Name string
}

var createMongoDBClusterTmpl = templateMustFromString(`
{
   	"name": "{{ .Name }}",
   	"environment": "PRESTABLE",
	"configSpec": {
		"mongodbSpec_4_2": {
			"mongod": {
				"resources": {
					"resourcePresetId": "s1.porto.1",
					"diskTypeId": "local-ssd",
                    "diskSize": {{ .DiskSize }}
				}
			}
		}
	},
	"databaseSpecs": [{
		"name": "testdb"
	}],
	"userSpecs": [{
		"name": "test",
		"password": "test_password"
	}],
  "hostSpecs": [{
	   "zoneId": "myt"
   }, {
	   "zoneId": "iva"
   }, {
	   "zoneId": "sas"
   }],
   "description": "test cluster"
}
`)

type CreateRedisClusterRequest struct {
	Name string
}

var createRedisClusterTmpl = templateMustFromString(`
{
   	"name": "{{ .Name }}",
	"environment": "PRESTABLE",
	"configSpec": {
		"redisConfig_6_2": {
			"password": "p@ssw#$rd!?",
			"databases": 15
		},
		"resources": {
			"resourcePresetId": "s1.porto.1",
            "diskSize": {{ .DiskSize }}
		}
	},
	"hostSpecs": [{
		"zoneId": "myt",
		"replicaPriority": 50
	}, {
		"zoneId": "iva"
	}, {
		"zoneId": "sas"
	}],
	"description": "test cluster",
   "networkId": "IN-PORTO-NO-NETWORK-API"
}
`)

var createGreenplumClusterTmpl = templateMustFromString(`
{
	"name": "{{ .Name }}",
	"folder_id": "folder1",
	"environment": "PRESTABLE",
	"description": "test cluster",
	"labels": {
		"foo": "bar"
	},
	"network_id": "network1",
	"config": {
		"zone_id": "myt",
		"subnet_id": "",
		"assign_public_ip": false
	},
	"master_config": {
		"resources": {
			"resourcePresetId": "s1.compute.2",
			"diskTypeId": "network-ssd",
            "diskSize": {{ .DiskSize }}
		}
	},
	"segment_config": {
		"resources": {
			"resourcePresetId": "s1.compute.1",
			"diskTypeId": "network-ssd",
            "diskSize": {{ .DiskSize }}
		}
	},
	"master_host_count": 2,
	"segment_in_host": 1,
	"segment_host_count": 4,
	"user_name": "usr1",
	"user_password": "Pa$$w0rd"
}
`)

var createElasticsearchClusterTmpl = templateMustFromString(`
{
	"name": "{{ .Name }}",
    "folder_id": "folder1",
    "environment": "PRESTABLE",
    "user_specs": [{
        "name": "donn",
        "password": "nomanisanisland100500"
    }, {
        "name": "frost",
        "password": "2roadsd1v3rgedInAYelloWood"
    }],
    "config_spec": {
        "edition": "platinum",
        "admin_password": "admin_password",
        "elasticsearch_spec": {
            "data_node": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "diskSize": {{ .DiskSize }}
                }
            },
            "master_node": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            }
        }
    },
    "host_specs": [{
        "zone_id": "myt",
        "type": "DATA_NODE"
    }],
    "description": "test cluster",
    "networkId": "network1"
}
`)

type UpdateBackupWindowRequest struct {
	Hours   int
	Minutes int
}

var updateBackupWindowTmpl = templateMustFromString(`
{
	"configSpec": {
		"backupWindowStart": {
			"hours": {{ .Hours }},
			"minutes": {{ .Minutes }}
		}
	}
}
`)

type AddShardRequest struct {
	ShardName string
}

var addShardTmpl = templateMustFromString(`
    {
        "shardName": "{{ .ShardName }}",
        "hostSpecs": [
            {
                "zoneId": "sas"
            }
        ]
    }
`)

var enableMongodbSharding = `
    {
        "mongos": {
			"resources": {
				"resourcePresetId": "s1.porto.1",
				"diskTypeId": "local-ssd",
				"diskSize": 10737418240
			}
        },
        "mongocfg": {
			"resources": {
				"resourcePresetId": "s1.porto.1",
				"diskTypeId": "local-ssd",
				"diskSize": 10737418240
			}
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
`

const defaultAPIToken = "rw-token"

func (c *Client) newRequestWithToken(token string) *resty.Request {
	req := c.client.R().
		SetHeader("X-YaCloud-SubjectToken", token).
		SetHeader("Accept", "application/json").
		SetHeader("Content-Type", "application/json")
	return req
}

func (c *Client) newRequest() *resty.Request {
	return c.newRequestWithToken("rw-token")
}

func getDiskSize(clusterType string) int64 {
	diskSize, ok := clusterDiskSize[clusterType]
	if ok {
		return diskSize
	}
	return defaultClusterDiskSize
}

type OperationMetadata struct {
	ClusterID string `json:"clusterId"`
}

type OperationResponse struct {
	ID       string            `json:"id"`
	Metadata OperationMetadata `json:"metadata"`
}

func (c *Client) IsReady(context.Context) error {
	resp, err := c.newRequest().Get(c.baseURL + "/ping")
	if err != nil {
		return err
	}
	if !resp.IsSuccess() {
		return xerrors.Errorf("bad status %s: %s", resp.Status(), resp.String())
	}
	return nil
}

func (c *Client) isSuccess(resp *resty.Response, err error) (OperationResponse, error) {
	if err != nil {
		return OperationResponse{}, xerrors.Errorf("fail while try create cluster: %w", err)
	}
	if resp.IsError() {
		return OperationResponse{}, xerrors.Errorf("int-api fail with %s: %s", resp.Status(), resp.String())
	}

	var op OperationResponse
	if err := json.Unmarshal(resp.Body(), &op); err != nil {
		return OperationResponse{}, xerrors.Errorf("response parse fail: %w", err)
	}
	return op, nil
}

func formatTemplateRequest(tmpl *template.Template, req interface{}) string {
	var createRequest strings.Builder
	if err := tmpl.Execute(&createRequest, req); err != nil {
		panic(xerrors.Errorf("unable to format create cluster request: %w", err))
	}
	return createRequest.String()
}

func (c *Client) CreatePGCluster(req CreatePostgresqlClusterRequest) (OperationResponse, error) {
	folderID := "folder1"
	if req.FolderID != "" {
		folderID = req.FolderID
	}
	if req.Token == "" {
		req.Token = defaultAPIToken
	}
	if req.ResourcePresetID == "" {
		req.ResourcePresetID = "s1.porto.1"
	}
	if req.DiskTypeID == "" {
		req.DiskTypeID = "local-ssd"
	}
	if len(req.Zones) == 0 {
		req.Zones = []string{"myt", "iva", "sas"}
	}
	if req.Environment == "" {
		req.Environment = "PRESTABLE"
	}
	if req.Description == "" {
		req.Description = "test cluster"
	}
	return c.isSuccess(c.newRequestWithToken(req.Token).SetBody(formatTemplateRequest(createPGClusterExtendedTmpl, req)).Post(
		c.baseURL + "/mdb/postgresql/1.0/clusters?folderId=" + folderID,
	))
}

func (c *Client) DeletePGCluster(clusterID string) (OperationResponse, error) {
	return c.isSuccess(
		c.newRequest().Delete(c.baseURL + "/mdb/postgresql/1.0/clusters/" + clusterID),
	)
}

func (c *Client) CreatePGHost(clusterID string) (OperationResponse, error) {
	return c.isSuccess(
		c.newRequest().SetBody(createHostJs).
			Post(c.baseURL + "/mdb/postgresql/1.0/clusters/" + clusterID + "/hosts:batchCreate"),
	)
}

func (c *Client) StopPGCluster(clusterID string) (OperationResponse, error) {
	return c.isSuccess(
		c.newRequest().Post(c.baseURL + "/mdb/postgresql/1.0/clusters/" + clusterID + ":stop"),
	)
}

func (c *Client) CreateClickHouseCluster(req CreateClickHouseClusterRequest) (OperationResponse, error) {
	if req.ResourcePresetID == "" {
		req.ResourcePresetID = "s1.porto.1"
	}
	if req.DiskTypeID == "" {
		req.DiskTypeID = "local-ssd"
	}
	if len(req.Zones) == 0 {
		req.Zones = []string{"man"}
	}
	return c.isSuccess(c.newRequest().SetBody(formatTemplateRequest(createClickHouseClusterExtendedTmpl, req)).Post(
		c.baseURL + "/mdb/clickhouse/1.0/clusters?folderId=folder1",
	))
}

func (c *Client) StopClickHouseCluster(clusterID string) (OperationResponse, error) {
	return c.isSuccess(
		c.newRequest().Post(c.baseURL + "/mdb/clickhouse/1.0/clusters/" + clusterID + ":stop"),
	)
}

func (c *Client) CreateCluster(clusterType string, req CreateClusterRequest) (OperationResponse, error) {
	createTmpl, ok := createClusterTemplates[clusterType]
	if !ok {
		return OperationResponse{}, fmt.Errorf("clusterCreate template was not found for: %s", clusterType)
	}
	if req.DiskSize == 0 {
		req.DiskSize = getDiskSize(clusterType)
	}
	return c.isSuccess(c.newRequest().SetBody(formatTemplateRequest(createTmpl, req)).Post(
		c.baseURL + fmt.Sprintf("/mdb/%s/1.0/clusters?folderId=folder1", clusterType),
	))
}

func (c *Client) CreateClusterGoAPI(clusterType string, req CreateClusterRequest, testContext *godogutil.TestContext, rawClient *grpc.ClientConn) (proto.Message, error) {
	createTmpl, ok := createClusterTemplatesGoAPI[clusterType]
	if !ok {
		return nil, fmt.Errorf("clusterCreate template was not found in go api for: %s", clusterType)
	}
	if req.DiskSize == 0 {
		req.DiskSize = getDiskSize(clusterType)
	}
	return grpcRequestWithData(
		"Create",
		fmt.Sprintf("yandex.cloud.priv.mdb.%s.v1.ClusterService", clusterType),
		formatTemplateRequest(createTmpl, req),
		testContext,
		rawClient,
	)
}

type ClusterResponse struct {
	ID     string `json:"id"`
	Name   string `json:"name"`
	Config struct {
		BackupWindowStart struct {
			Hours   int `json:"hours"`
			Minutes int `json:"minutes"`
		}
	} `json:"config"`
}

func (c *Client) GetCluster(clusterID, clusterType string) (ClusterResponse, error) {
	resp, err := c.newRequest().Get(c.baseURL + fmt.Sprintf("/mdb/%s/1.0/clusters/%s", clusterType, clusterID))
	if err != nil {
		return ClusterResponse{}, xerrors.Errorf("fail while try get cluster: %w", err)
	}
	if resp.IsError() {
		return ClusterResponse{}, xerrors.Errorf("int-api fail with %s: %s", resp.Status(), resp.String())
	}

	var cluster ClusterResponse
	if err := json.Unmarshal(resp.Body(), &cluster); err != nil {
		return ClusterResponse{}, xerrors.Errorf("response parse fail: %w", err)
	}
	return cluster, nil
}

func (c *Client) UpdateBackupWindow(clusterID, clusterType string, req UpdateBackupWindowRequest) (OperationResponse, error) {
	return c.isSuccess(c.newRequest().SetBody(formatTemplateRequest(updateBackupWindowTmpl, req)).Patch(
		c.baseURL + fmt.Sprintf("/mdb/%s/1.0/clusters/%s", clusterType, clusterID),
	))
}

func (c *Client) EnableMongoDBSharding(clusterID string) (OperationResponse, error) {
	return c.isSuccess(c.newRequest().SetBody(enableMongodbSharding).Post(
		c.baseURL + fmt.Sprintf("/mdb/mongodb/1.0/clusters/%s:enableSharding", clusterID),
	))
}

func (c *Client) AddShard(clusterID, clusterType string, req AddShardRequest) (OperationResponse, error) {
	return c.isSuccess(c.newRequest().SetBody(formatTemplateRequest(addShardTmpl, req)).Post(
		c.baseURL + fmt.Sprintf("/mdb/%s/1.0/clusters/%s/shards", clusterType, clusterID),
	))
}

type ListBackupsResponse struct {
	Backups []struct {
		CreatedAt        string   `json:"createdAt"`
		FolderID         string   `json:"folderId"`
		ID               string   `json:"id"`
		Size             int      `json:"size"`
		SourceClusterID  string   `json:"sourceClusterId"`
		SourceShardNames []string `json:"sourceShardNames"`
		StartedAt        string   `json:"startedAt"`
		Type             string   `json:"type"`
	} `json:"backups"`
}

func (c *Client) ListBackups(clusterID, clusterType string) (ListBackupsResponse, error) {
	resp, err := c.newRequest().Get(c.baseURL + fmt.Sprintf("/mdb/%s/1.0/clusters/%s/backups", clusterType, clusterID))
	if err != nil {
		return ListBackupsResponse{}, xerrors.Errorf("fail while try list cluster backups: %w", err)
	}
	list := ListBackupsResponse{}
	if err := json.Unmarshal(resp.Body(), &list); err != nil {
		return ListBackupsResponse{}, xerrors.Errorf("response parse fail: %w", err)
	}
	return list, nil
}

func (c *Client) AddZooKeeper(clusterID string) (OperationResponse, error) {
	return c.isSuccess(c.newRequest().Post(
		c.baseURL + fmt.Sprintf("/mdb/clickhouse/1.0/clusters/%s:addZookeeper", clusterID),
	))
}

func grpcRequestWithData(method, service string, body string, testContext *godogutil.TestContext, rawClient *grpc.ClientConn) (proto.Message, error) {
	ctx := testContext.Context()

	rc := grpcreflect.NewClient(ctx, reflectpb.NewServerReflectionClient(rawClient))
	sd, err := rc.ResolveService(service)
	if err != nil {
		return nil, xerrors.Errorf("failed to resolve service %q: %w", service, err)
	}

	md := sd.FindMethodByName(method)
	if md == nil {
		return nil, xerrors.Errorf("failed to find requested method %q in service %q", method, service)
	}

	s := grpcdynamic.NewStub(rawClient)
	req := dynamic.NewMessage(md.GetInputType())
	if err := jsonpb.UnmarshalString(body, req); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal json %s into protobuf: %w", body, err)
	}

	return s.InvokeRpc(ctx, md, req)
}
