package main

import (
	"context"
	"log"
	"net"
	"sort"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/dbaasutil"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/mysqlutil"
)

var verbose bool
var timeout time.Duration
var dbaasConfig string
var defaultsFile string

func init() {
	pflag.BoolVarP(&verbose, "verbose", "v", false, "Some logging")
	pflag.DurationVarP(&timeout, "timeout", "t", 30*time.Second, "Timeout to resolve hosts, 30s, 1m, etc")
	pflag.StringVar(&dbaasConfig, "dbaas-config", "/etc/dbaas.conf", "dbaas cluster config file")
	pflag.StringVar(&defaultsFile, "defaults-file", "", "mysql client settings file")
}

func readCache(ctx context.Context, db *sqlx.DB) (map[string][]string, error) {
	cache := make(map[string][]string)
	rows, err := db.QueryxContext(ctx, `
		SELECT IFNULL(HOST, "NULL"), IP
		FROM performance_schema.host_cache
	`)
	if err != nil {
		return nil, err
	}
	defer func() { _ = rows.Close() }()
	for rows.Next() {
		var host, ip string
		err = rows.Scan(&host, &ip)
		if err != nil {
			return nil, err
		}
		cache[host] = append(cache[host], ip)
	}
	return cache, nil
}

func resolveHosts(ctx context.Context, hosts []string) map[string][]string {
	type res struct {
		host  string
		addrs []string
	}
	resChan := make(chan res, len(hosts))
	for _, host := range hosts {
		go func(host string) {
			addrs, err := net.DefaultResolver.LookupHost(ctx, host)
			if err != nil {
				// if DNS lookup fails, it's better to stop and don't clear DNS caches
				log.Fatalf("failed to resolve %s: %v", host, err)
			}
			sort.Strings(addrs)
			resChan <- res{host, addrs}
		}(host)
	}
	actual := make(map[string][]string)
	for i := 0; i < len(hosts); i++ {
		res := <-resChan
		actual[res.host] = res.addrs
	}
	return actual
}

func reverse(src map[string][]string) map[string][]string {
	res := make(map[string][]string)
	for k, vs := range src {
		for _, v := range vs {
			res[v] = append(res[v], k)
		}
	}
	return res
}

func stringInSlice(a string, list []string) bool {
	for _, b := range list {
		if b == a {
			return true
		}
	}
	return false
}

func isCacheCorrect(cache, actual map[string][]string) bool {
	correct := true
	reversedCache := reverse(cache)
	reversedActual := reverse(actual)
	for host, cachedIPs := range cache {
		if actualIPs, ok := actual[host]; ok {
			for _, cachedIP := range cachedIPs {
				if !stringInSlice(cachedIP, actualIPs) {
					log.Printf("ip mismatch for %s: actual=%#v cached=%#v", host, actualIPs, cachedIPs)
					correct = false
				}
			}
		}
	}
	for ip, cachedHosts := range reversedCache {
		if actualHosts, ok := reversedActual[ip]; ok {
			for _, cachedHost := range cachedHosts {
				if !stringInSlice(cachedHost, actualHosts) {
					log.Printf("host mismatch for %s: actual=%s cached=%s", ip, actualHosts, cachedHosts)
					correct = false
				}
			}
		}
	}
	return correct
}

func clearCache(ctx context.Context, db *sqlx.DB) {
	_, err := db.ExecContext(ctx, "FLUSH HOSTS")
	if err != nil {
		log.Fatalf("faile to flush mysql hosts: %v", err)
	}
	if verbose {
		log.Printf("mysql hosts flushed")
	}
}

func main() {
	pflag.Parse()
	ctx := context.Background()
	ctx = signals.WithCancelOnSignal(ctx)
	ctx, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()
	db, err := mysqlutil.ConnectWithDefaultsFile(defaultsFile)
	if err != nil {
		log.Fatalf("failed to create mysql connection: %v", err)
	}
	defer func() { _ = db.Close() }()
	config, err := dbaasutil.ReadDbaasConfig(dbaasConfig)
	if err != nil {
		log.Fatal(err)
	}
	hosts := config.ClusterHosts
	if verbose {
		log.Printf("cluster hosts: %v", hosts)
	}
	cache, err := readCache(ctx, db)
	if err != nil {
		log.Fatal(err)
	}
	if verbose {
		log.Printf("cached ips: %+v", cache)
		log.Printf("reversed cached ips: %+v", reverse(cache))
	}
	actual := resolveHosts(ctx, hosts)
	if verbose {
		log.Printf("actual ips: %+v", actual)
		log.Printf("reversed actual ips: %+v", reverse(actual))
	}
	if isCacheCorrect(cache, actual) {
		log.Printf("cache is correct")
		return
	}
	clearCache(ctx, db)
}
