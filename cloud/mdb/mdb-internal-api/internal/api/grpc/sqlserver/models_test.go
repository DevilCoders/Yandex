package sqlserver

import (
	"testing"

	"github.com/golang/protobuf/ptypes/wrappers"
	"github.com/stretchr/testify/assert"

	config "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1/config"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
)

func TestConfigFromSpecNothingFromGrpc(t *testing.T) {
	spec := &config.SQLServerConfig2016Sp2Ent{}
	defaultConfig := &ssmodels.ConfigBase{}
	conf := &ssmodels.ConfigBase{}
	configFromGRPC(spec, conf, grpc.FieldNamesFromGRPCPaths(spec, grpc.AllPaths()))
	assert.Equal(t, defaultConfig, conf)
}

func TestConfigFromSpecOneOptionFromGrpc(t *testing.T) {
	spec := &config.SQLServerConfig2016Sp2Ent{
		MaxDegreeOfParallelism: &wrappers.Int64Value{Value: 10},
	}
	defaultConfig := &ssmodels.ConfigBase{}
	conf := &ssmodels.ConfigBase{}
	configFromGRPC(spec, conf, grpc.FieldNamesFromGRPCPaths(spec, grpc.AllPaths()))
	assert.Equal(t, conf.MaxDegreeOfParallelism.Valid, true)
	assert.Equal(t, conf.MaxDegreeOfParallelism.Int64, spec.MaxDegreeOfParallelism.Value)
	conf.MaxDegreeOfParallelism = defaultConfig.MaxDegreeOfParallelism
	assert.Equal(t, conf, defaultConfig)
}

func TestConfigFromSpecSeveralOptionsFromGrpc(t *testing.T) {
	spec := &config.SQLServerConfig2016Sp2Ent{
		MaxDegreeOfParallelism: &wrappers.Int64Value{Value: 17},
		AuditLevel:             &wrappers.Int64Value{Value: 2},
	}
	defaultConfig := &ssmodels.ConfigBase{}
	conf := &ssmodels.ConfigBase{}
	configFromGRPC(spec, conf, grpc.FieldNamesFromGRPCPaths(spec, grpc.AllPaths()))
	assert.Equal(t, conf.MaxDegreeOfParallelism.Valid, true)
	assert.Equal(t, conf.MaxDegreeOfParallelism.Int64, spec.MaxDegreeOfParallelism.GetValue())
	assert.Equal(t, conf.AuditLevel.Valid, true)
	assert.Equal(t, conf.AuditLevel.Int64, spec.AuditLevel.GetValue())
	conf.MaxDegreeOfParallelism = defaultConfig.MaxDegreeOfParallelism
	conf.AuditLevel = defaultConfig.AuditLevel
	assert.Equal(t, conf, defaultConfig)
}

func TestConfigToSpecNothingFromConfig(t *testing.T) {
	emptySpec := &config.SQLServerConfig2016Sp2Ent{}
	conf := &ssmodels.ConfigBase{}
	spec := &config.SQLServerConfig2016Sp2Ent{}
	configToGRPC(conf, spec)
	assert.Equal(t, spec, emptySpec)
}

func TestConfigToSpecOneOptionFromConfig(t *testing.T) {
	conf := &ssmodels.ConfigBase{
		MaxDegreeOfParallelism: optional.NewInt64(10),
	}
	defaultSpec := &config.SQLServerConfig2016Sp2Ent{}
	spec := &config.SQLServerConfig2016Sp2Ent{}
	configToGRPC(conf, spec)
	assert.Equal(t, spec.MaxDegreeOfParallelism.Value, conf.MaxDegreeOfParallelism.Int64)
	spec.MaxDegreeOfParallelism = defaultSpec.MaxDegreeOfParallelism
	assert.Equal(t, spec, defaultSpec)
}

func TestConfigToSpecSeveralOptionsFromConfig(t *testing.T) {
	conf := &ssmodels.ConfigBase{
		MaxDegreeOfParallelism: optional.NewInt64(17),
		AuditLevel:             optional.NewInt64(2),
	}
	defaultSpec := &config.SQLServerConfig2016Sp2Ent{}
	spec := &config.SQLServerConfig2016Sp2Ent{}
	configToGRPC(conf, spec)
	assert.Equal(t, spec.MaxDegreeOfParallelism.Value, conf.MaxDegreeOfParallelism.Int64)
	assert.Equal(t, spec.AuditLevel.Value, conf.AuditLevel.Int64)
	spec.MaxDegreeOfParallelism = defaultSpec.MaxDegreeOfParallelism
	spec.AuditLevel = defaultSpec.AuditLevel
	assert.Equal(t, spec, defaultSpec)
}

func TestConfigSetToToSpec(t *testing.T) {
	defaultConf := &ssmodels.ConfigBase{}
	conf := &ssmodels.ConfigBase{
		MaxDegreeOfParallelism: optional.NewInt64(17),
		AuditLevel:             optional.NewInt64(2),
	}
	configSet := ssmodels.ConfigSetSQLServer{
		DefaultConfig:   defaultConf,
		UserConfig:      conf,
		EffectiveConfig: conf,
	}
	spec := &config.SQLServerConfigSet2016Sp2Ent{}
	configSetToGRPC(configSet, spec)
	assert.NotNil(t, spec.DefaultConfig)
	// TODO: enable after setting default config
	//assert.NotNil(t, spec.DefaultConfig.MaxDegreeOfParallelism)
	//assert.Equal(t, 0, spec.DefaultConfig.MaxDegreeOfParallelism.Value)
	//assert.NotNil(t, spec.DefaultConfig.AuditLevel)
	//assert.Equal(t, 0, spec.DefaultConfig.AuditLevel.Value)
	assert.NotNil(t, spec.UserConfig)
	assert.NotNil(t, spec.UserConfig.MaxDegreeOfParallelism)
	assert.Equal(t, conf.MaxDegreeOfParallelism.Int64, spec.UserConfig.MaxDegreeOfParallelism.Value)
	assert.NotNil(t, spec.UserConfig.AuditLevel)
	assert.Equal(t, conf.AuditLevel.Int64, spec.UserConfig.AuditLevel.Value)
	assert.NotNil(t, spec.EffectiveConfig)
	assert.NotNil(t, spec.EffectiveConfig.MaxDegreeOfParallelism)
	assert.Equal(t, conf.MaxDegreeOfParallelism.Int64, spec.EffectiveConfig.MaxDegreeOfParallelism.Value)
	assert.NotNil(t, spec.EffectiveConfig.AuditLevel)
	assert.Equal(t, conf.AuditLevel.Int64, spec.EffectiveConfig.AuditLevel.Value)
}
