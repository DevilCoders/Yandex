package clickhouse

import (
	"crypto/tls"
	"crypto/x509"
	"database/sql"
	"fmt"
	"log"
	"sync"

	"github.com/ClickHouse/clickhouse-go"
	"github.com/yandex/pandora/core"
	"github.com/yandex/pandora/core/aggregator/netsample"

	. "a.yandex-team.ru/cloud/mdb/benchmarks/pandora/internal/cert"
)

type Ammo struct {
	Script string
	Aid    uint32
	Bid    uint32
	Tid    uint32
	Delta  int32
}

type GunConfig struct {
	Target   string `validate: "required"`
	Port     uint16 `validate: "required"`
	User     string `validate: "required"`
	Password string `validate: "required"`
	Database string `validate: "required"`
}

type Gun struct {
	conf    GunConfig
	connect *sql.DB
	aggr    core.Aggregator
	deps    core.GunDeps
}

var mutext = &sync.Mutex{}

func NewGun(conf GunConfig) *Gun {
	return &Gun{conf: conf}
}

func (g *Gun) Bind(aggr core.Aggregator, deps core.GunDeps) error {
	var err error
	mutext.Lock()
	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM([]byte(YandexInternalRootCA))
	clickhouse.RegisterTLSConfig("custom", &tls.Config{
		RootCAs: caCertPool,
	})
	connString := fmt.Sprintf("tcp://%s:%d?username=%s&password=%s&tls_config=custom&debug=true", g.conf.Target, g.conf.Port, g.conf.User, g.conf.Password)
	log.Println(fmt.Sprintf("Connection %s", connString))
	connect, err := sql.Open("clickhouse", connString)
	g.connect = connect
	mutext.Unlock()

	if err != nil {
		log.Fatal("Unable to open connection: ", err)
	}
	g.aggr = aggr
	g.deps = deps
	if err != nil {
		return err
	} else {
		err = connect.Ping()
		if err != nil {
			log.Fatal("Unable to ping: ", err)
		}
		return err
	}
}

func (g *Gun) Shoot(ammo core.Ammo) {
	g.shoot(ammo.(*Ammo))
}

func (g *Gun) shoot(ammo *Ammo) {
	switch ammo.Script {
	case "select1":
		g.Select1(ammo)
	default:
		sample := netsample.Acquire("none")
		sample.SetProtoCode(200)
		g.aggr.Report(sample)
	}
}
