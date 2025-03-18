package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/labstack/echo/v4"
	"golang.org/x/sync/errgroup"
	"io"
	"log"
	"net/http"
	"os"
	"os/signal"
	"strings"
	"sync/atomic"
	"syscall"
	"time"

	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver"
	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver/cachedresolver"
	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver/cacher"
	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver/httpresolver"
)

type Stat struct {
	Service string
	Signal  string
	Count   int64
}

type RequestInfo struct {
	Service string
	Host    string
	Count   int64
	Time    time.Time
}

func (s *Stat) UnmarshalJSON(data []byte) error {
	var v []interface{}
	if err := json.Unmarshal(data, &v); err != nil {
		return err
	}

	id := v[0].(string)
	fields := strings.Split(id, ";")
	if len(fields) != 2 {
		return fmt.Errorf("id don't contain 2 fields: [%s]", id)
	}

	serviceTypeName := strings.Split(fields[0], "=")
	if len(serviceTypeName) != 2 || serviceTypeName[0] != "service_type" {
		return fmt.Errorf("prefix not equal 'service_type=': [%s]", serviceTypeName)
	}

	s.Service = serviceTypeName[1]
	s.Signal = fields[1]
	s.Count = int64(v[1].(float64))

	return nil
}

func fetchAll(ctx context.Context, endpoints []*resolver.Endpoint, services map[string]bool) ([]RequestInfo, error) {
	ctx, cancel := context.WithTimeout(ctx, 3*time.Second)
	g, ctx := errgroup.WithContext(ctx)
	defer cancel()

	var serviceStats []RequestInfo

	numSuccess := new(uint64)

	// run all the http requests in parallel
	for _, endpoint := range endpoints {
		host := endpoint.FQDN
		g.Go(func() error {
			errChan := make(chan error)
			resultChain := make(chan RequestInfo)
			go func(host string) {
				url := "http://" + host + ":13515/unistats_lw"
				req, err := http.NewRequestWithContext(ctx, "GET", url, nil)
				if err != nil {
					errChan <- err
					return
				}
				// req.Close = true
				res, err := http.DefaultClient.Do(req)
				if err != nil {
					errChan <- err
					return
				}
				// defer res.Body.Close()

				body, err := io.ReadAll(res.Body)
				if err != nil {
					errChan <- err
					return
				}
				var stat []Stat
				if err := json.Unmarshal(body, &stat); err != nil {
					errChan <- err
					return
				}

				for _, s := range stat {
					if _, exists := services[s.Service]; exists && s.Signal == "requests_deee" {
						v := RequestInfo{Service: s.Service, Host: host, Count: s.Count, Time: time.Now()}
						resultChain <- v
					}
				}
				atomic.AddUint64(numSuccess, 1)
				errChan <- nil
			}(host)

			for {
				select {
				case <-ctx.Done():
					return ctx.Err()
				case /* err := */ <-errChan:
					/*
						if err != nil {
							return fmt.Errorf("fetch: %w", err)
						}
					*/
					return nil
				case v := <-resultChain:
					serviceStats = append(serviceStats, v)
				}
			}
		})
	}

	// Wait for completion and return the first error (if any)
	return serviceStats, g.Wait()
}

func main() {
	var config AppConfig
	var configPath string
	flag.StringVar(&configPath, "c", "config.yml", "Path to config file")
	flag.Parse()

	if err := LoadConfigFromFile(configPath, &config); err != nil {
		log.Fatal(err)
	}

	r, err := httpresolver.New()
	if err != nil {
		log.Fatal(err)
	}

	c := cacher.NewInMemory(cacher.InMemoryObjectTTL(10 * time.Second))
	// данная опция позволяет использовать объект из кеша с истекшим сроком жизни
	// в случае если произошла ошибка при запросе к YP SD
	useStaleCache := cachedresolver.ReturnStaleCache(true)
	cr, err := cachedresolver.New(r, c, useStaleCache)
	if err != nil {
		log.Fatal(err)
	}

	httpHandler := echo.New()
	httpHandler.GET("/ping", NewHealthCheckHTTPHandler())
	httpCtx, err := RunServer(config.HTTPServer, httpHandler)
	if err != nil {
		log.Fatal(err)
	}

	ctx := context.Background()

	services := make(map[string]bool)
	limits := make(map[string]map[string]int64)
	ruchkas := make(map[string]string)

	for _, r := range config.Ruchkas {
		ruchkas[r.Name] = r.Ruchka
	}

	for _, s := range config.Services {
		services[s.Service] = true

		its := make(map[string]int64)
		for _, i := range s.Its {
			its[i.Ruchka] = i.Limit
		}

		limits[s.Service] = make(map[string]int64)
		limits[s.Service] = its
	}

	captchaDuration, err := time.ParseDuration(config.CaptchaDuration)
	if err != nil {
		log.Fatal(err)
	}

	its, err := NewITS(config.NannyTokenFilename, captchaDuration, services, ruchkas)
	if err != nil {
		log.Fatal(err)
	}

	http.DefaultClient.Timeout = time.Second
	go func() {
		var prevCounters []RequestInfo
		hasPrev := false
		for {
			endpoints, err := ResolveAllEndpoints(ctx, cr, config.Endpoints)
			if err != nil {
				log.Fatal(err)
			}

			if counters, err := fetchAll(ctx, endpoints, services); err != nil {
				log.Println(err)
			} else {
				numAnswers := make(map[string]bool)
				for _, c := range counters {
					numAnswers[c.Host] = true
				}

				ratio := float64(len(numAnswers)) / float64(len(endpoints))
				if ratio > 0.5 {
					if hasPrev {
						if rps, err := getRps(counters, prevCounters); err != nil {
							log.Printf("getRps fail: %s", err.Error())
							hasPrev = false
							prevCounters = counters
						} else {
							for s := range services {
								for r, limit := range limits[s] {
									if time.Now().After(its.Config[s].CaptchaTill) && its.Config[s].AllPressed() {
										log.Printf("Current value: %d rps in service %s-%s, detected crit value %d",
											rps[s], s, r, limit)
										its.Disable(s, r)
									} else if rps[s] < limit {
										log.Printf("Current rps for %s-%s: %d, crit %d", s, r, rps[s], limit)
									}

									if rps[s] > limit && time.Now().After(its.Config[s].CaptchaTill) {
										log.Printf("Current value: %d rps in service %s-%s, DDoS detected crit value %d",
											rps[s], s, r, limit)
										its.Enable(s, r)
									}

									if time.Now().Before(its.Config[s].CaptchaTill) {
										if rps[s] >= limit && its.Config[s].AllPressed() {
											its.Config[s].CaptchaTill = time.Now().Add(its.CaptchaDuration)
										}

										log.Printf(
											"Captcha for %s-%s wil turn off after %ds\n",
											s, r, int64(time.Until(its.Config[s].CaptchaTill).Seconds()),
										)
									}
								}
							}
						}
					} else {
						hasPrev = true
					}
					prevCounters = counters
				}
			}
			time.Sleep(time.Second)
		}
	}()

	defer func() {
		if err := httpCtx.Shutdown(ctx); err != nil {
			log.Printf("httpCtx Shutdown error: [%s]", err)
		}
	}()

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGTERM, syscall.SIGINT)

	select {
	case <-quit:
		log.Println("Quit")
	case <-httpCtx.Stopped():
		log.Fatal(httpCtx.Err())
	}
}

func getRps(counters []RequestInfo, prevCounters []RequestInfo) (map[string]int64, error) {
	rps := make(map[string]int64)

	countersMap := make(map[string]RequestInfo)
	for _, r := range counters {
		countersMap[r.Service+r.Host] = r
	}

	prevCountersMap := make(map[string]RequestInfo)
	for _, r := range prevCounters {
		prevCountersMap[r.Service+r.Host] = r
	}

	for k, v := range countersMap {
		if prevVal, ok := prevCountersMap[k]; ok {
			rps[v.Service] += (v.Count - prevVal.Count) * 1000 / (v.Time.Sub(prevVal.Time).Milliseconds())
		}
	}

	return rps, nil
}
