package mongoclient

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/library/go/core/log"

	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type Collections struct {
	Services string
	Shifts   string
	Cache    string
	Template string
	User     string
}

type Config struct {
	Log log.Logger
	db  *mongo.Database

	Username    string `config:"mongo-user,required"`
	Password    string `config:"mongo-password,required"`
	Host        string `config:"mongo-host,required"`
	Port        uint
	Database    string
	ReplicaSet  string
	CAFile      string `config:"mongo-ca-file"`
	Collections Collections
}

func DefaultConfig() *Config {
	return &Config{
		Port:       27018,
		Database:   "db1",
		ReplicaSet: "rs01",
		Collections: Collections{
			Services: "services",
			Shifts:   "shift",
			Cache:    "cache",
			Template: "template",
			User:     "user",
		},
	}
}

func (c *Config) InitDBConnection() (err error) {
	if len(c.Password) == 0 {
		return fmt.Errorf("mongo auth failed: password") // TODO: Config checks !!!!
	}
	if len(c.Host) == 0 {
		return fmt.Errorf("mongo auth failed: host")
	}
	if len(c.Username) == 0 {
		return fmt.Errorf("mongo auth failed: user")
	}

	var conn string
	for _, h := range strings.Split(c.Host, ",") {
		conn += fmt.Sprintf("%s:%d,", h, c.Port)
	}

	uri := fmt.Sprintf("mongodb://%s:%s@%s/%s?replicaSet=%s",
		c.Username,
		c.Password,
		conn[:len(conn)-1],
		c.Database,
		c.ReplicaSet,
	)
	if len(c.CAFile) != 0 {
		if !strings.ContainsRune(uri, '?') {
			if uri[len(uri)-1] != '/' {
				uri += "/"
			}

			uri += "?"
		} else {
			uri += "&"
		}

		uri += "ssl=true&sslCertificateAuthorityFile=" + c.CAFile
	}

	client, err := mongo.Connect(context.Background(), options.Client().ApplyURI(uri).SetMaxPoolSize(250))
	if err != nil {
		panic(err)
	}

	err = client.Ping(context.Background(), nil)
	if err != nil {
		panic(err)
	}

	c.db = client.Database(c.Database)
	return
}
