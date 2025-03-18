package main

import (
	"bytes"
	"context"
	"crypto/tls"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver"
	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver/grpcresolver"
	"a.yandex-team.ru/library/go/certifi"
)

var StablePODSET string
var PrestablePODSET string

var Timeout time.Duration
var CaptchaDuration time.Duration

var NannyToken string
var Clusters = []string{"man", "sas", "vla"}
var ITSRuchkas = []string{"antirobot_cbb_panic_mode_for_", "antirobot_suspicious_ban_for_"}
var CTypes = []string{"stable", "prestable"}

var LogFile = "../../daemon/logs/ddos_detector_go.log"

var infoLogger *log.Logger
var errorLogger *log.Logger

// vvvvvvvvvvvvvvvvvvvvvvvvvv
var Config = []ServiceConfig{ // add new services here
	NewServiceConfig("captcha_gen", 10),
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^

type ServiceConfig struct {
	Service     string
	Limit       int64
	Etag        map[string]string
	CaptchaTill time.Time
}

func InitLogger() {
	f, err := os.OpenFile(LogFile, os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		log.Fatal(err)
	}

	infoLogger = log.New(f, "INFO\t", log.Ldate|log.Ltime)
	errorLogger = log.New(f, "ERROR\t", log.Ldate|log.Ltime)
}

func CreateClient() http.Client {
	tlsConfig := &tls.Config{}
	certPool, err := certifi.NewCertPool()
	if err != nil {
		errorLogger.Fatalf("failed to create cert pool: %v\n", err)
	} else {
		tlsConfig.RootCAs = certPool
	}

	return http.Client{
		Transport: &http.Transport{
			Dial:            DialTimeout,
			TLSClientConfig: tlsConfig,
		},
	}
}

func NewServiceConfig(service string, limit int64) ServiceConfig {
	obj := ServiceConfig{
		Service:     service,
		Limit:       limit,
		CaptchaTill: time.Now(),
		Etag:        make(map[string]string),
	}

	for _, ctype := range CTypes {
		for _, ruchka := range ITSRuchkas {
			obj.Etag[ruchka+ctype] = ""
		}
	}
	return obj
}

func (obj *ServiceConfig) AllRuchkasPressed() bool {
	for _, value := range obj.Etag {
		if len(value) == 0 {
			return false
		}
	}
	return true
}

func LoadNannyToken() {
	f, err := os.Open("../../daemon/secrets/NANNY_OAUTH_TOKEN")
	if err != nil {
		errorLogger.Fatal(err)
	}

	defer func() {
		if err = f.Close(); err != nil {
			errorLogger.Fatal(err)
		}
	}()

	content, err := ioutil.ReadAll(f)
	if err != nil {
		errorLogger.Fatal(err)
	}

	NannyToken = string(content)
}

func DialTimeout(network, addr string) (net.Conn, error) {
	return net.DialTimeout(network, addr, Timeout)
}

func EnableITS(service string, ctype string, ruchka string) (etag string, content string, statusCode int) {
	client := CreateClient()
	itsURL := fmt.Sprintf("https://its.yandex-team.ru/v1/values/antirobot/antirobot/%s/%s/%s/", service, ctype, ruchka+service)

	req, _ := http.NewRequest("POST", itsURL, bytes.NewBuffer([]byte(`{"value":"`+service+`"}`)))
	req.Header.Set("Authorization", "Oauth "+NannyToken)
	req.Header.Set("Content-Type", "text/plain")

	resp, err := client.Do(req)

	if err != nil {
		errorLogger.Printf("EnableITS failed: %v", err)
		return
	}

	etag = resp.Header.Get("ETag")
	body, err := ioutil.ReadAll(resp.Body)
	defer resp.Body.Close()
	if err != nil {
		errorLogger.Fatal(err)
	}

	content = string(body)
	statusCode = resp.StatusCode
	return
}

func DisableITS(service string, etag string, ctype string, ruchka string) (content string, statusCode int) {
	client := CreateClient()
	itsURL := fmt.Sprintf("https://its.yandex-team.ru/v1/values/antirobot/antirobot/%s/%s/%s/", service, ctype, ruchka+service)

	req, _ := http.NewRequest(http.MethodDelete, itsURL, nil)
	req.Header.Set("Authorization", "Oauth "+NannyToken)
	req.Header.Set("Content-Type", "text/plain")
	req.Header.Set("If-Match", etag)

	resp, err := client.Do(req)
	if err != nil {
		errorLogger.Fatal(err)
	}

	body, err := ioutil.ReadAll(resp.Body)
	defer resp.Body.Close()
	if err != nil {
		errorLogger.Fatal(err)
	}

	content = string(body)
	statusCode = resp.StatusCode
	return
}

func ResolveEndpoints(PODSET string) (endpoints []*resolver.Endpoint) {
	endpoints = make([]*resolver.Endpoint, 0)
	r, _ := grpcresolver.New()
	for _, cluster := range Clusters {
		ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
		defer cancel()

		resp, err := r.ResolveEndpoints(ctx, cluster, PODSET+cluster)
		if err != nil {
			errorLogger.Fatal(err)
		}
		if resp.ResolveStatus != resolver.StatusEndpointNotExists &&
			resp.EndpointSet != nil {
			endpoints = append(endpoints, resp.EndpointSet.Endpoints...)
		}
	}
	return
}

func ResolveAllEndpoints() (endpoints []*resolver.Endpoint) {
	endpoints = ResolveEndpoints(PrestablePODSET)
	endpoints = append(endpoints, ResolveEndpoints(StablePODSET)...)
	return endpoints
}

func AskInstance(url string) (instanceBasedRPS map[string]int64) {
	instanceBasedRPS = make(map[string]int64)
	result, err := http.Get(url)
	if err != nil {
		infoLogger.Printf("%v\n", err)
		return
	}
	defer result.Body.Close()
	body, err := ioutil.ReadAll(result.Body)
	if err != nil {
		infoLogger.Printf("%v\n", err)
		return
	}
	str := string(body)

	tokens := strings.FieldsFunc(str, func(ch rune) bool {
		return ch == '[' || ch == ']' || ch == ',' || ch == ';' || ch == '"' || ch == '='
	})

	for i := 3; i < len(tokens); i += 4 {
		rpsValue, err := strconv.ParseInt(strings.Trim(tokens[i], " "), 10, 64)
		if err != nil {
			infoLogger.Printf("%v\n", err)
			return
		}
		instanceBasedRPS[strings.Trim(tokens[i-2], " ")] = rpsValue
	}
	return
}

func GetRPS(endpoints []*resolver.Endpoint) (serviceRPS map[string]*int64) {
	serviceRPS = make(map[string]*int64)
	for i := range Config {
		serviceRPS[Config[i].Service] = new(int64)
	}
	waitGroups := sync.WaitGroup{}
	for i := range endpoints {
		waitGroups.Add(1)
		go func(url string) {
			instanceBasedRPS := AskInstance("http://" + url + ":13515/unistats_lw")
			for i := range Config {
				atomic.AddInt64(serviceRPS[Config[i].Service], instanceBasedRPS[Config[i].Service])
			}
			waitGroups.Add(-1)
		}(endpoints[i].FQDN)
	}
	waitGroups.Wait()
	return
}

func RunDetector() {
	endpoints := ResolveAllEndpoints()
	start := time.Now()
	prevEnd := time.Now()
	previousRPS := make(map[string]*int64)
	hasPreviousRPS := false
	for {
		duration := time.Since(start)
		if duration >= time.Minute*10 {
			endpoints = ResolveAllEndpoints()
			start = time.Now()
		}
		serviceRPS := GetRPS(endpoints)
		if hasPreviousRPS {
			realRPS := make(map[string]int64)
			for service, RPS := range serviceRPS {
				if *RPS < *previousRPS[service] {
					realRPS[service] = *RPS
				} else {
					realRPS[service] = (*RPS - *previousRPS[service]) * 1000 / int64(time.Since(prevEnd).Milliseconds())
				}
			}

			for i := range Config {
				rps := realRPS[Config[i].Service]

				if time.Now().After(Config[i].CaptchaTill) && Config[i].AllRuchkasPressed() {
					for _, ctype := range CTypes {
						for _, ruchka := range ITSRuchkas {
							response, statusCode := DisableITS(Config[i].Service, Config[i].Etag[ruchka+ctype], ctype, ruchka)

							infoLogger.Printf(
								"Hide captcha for its: %s ({%s}{%s}), status code: {%d}, response: [{%s}]\n",
								Config[i].Service, ruchka, ctype, statusCode, response,
							)

							Config[i].Etag[ruchka+ctype] = ""
						}
					}
				} else if rps < Config[i].Limit {
					infoLogger.Printf(
						"Current rps for %s: %d, crit: %d. This is fine!\n",
						Config[i].Service, rps, Config[i].Limit,
					)
				}

				if rps >= Config[i].Limit && time.Now().After(Config[i].CaptchaTill) {
					infoLogger.Printf(
						"Current value: %d rps in service_type %s. DDoS detected (crit value: %d)! Captcha turns on!\n",
						rps, Config[i].Service, Config[i].Limit,
					)

					for _, ctype := range CTypes {
						for _, ruchka := range ITSRuchkas {
							var response string
							var statusCode int
							Config[i].Etag[ruchka+ctype], response, statusCode = EnableITS(Config[i].Service, ctype, ruchka)

							infoLogger.Printf(
								"Show captcha its: %s (%s%s), status code: %d, response: [%s]\n",
								Config[i].Service, ruchka, ctype, statusCode, response,
							)

							if len(Config[i].Etag[ruchka+ctype]) == 0 {
								break
							} else {
								Config[i].CaptchaTill = time.Now().Add(CaptchaDuration)
							}
						}
					}
				}

				if time.Now().Before(Config[i].CaptchaTill) {
					if rps >= Config[i].Limit && Config[i].AllRuchkasPressed() {
						Config[i].CaptchaTill = time.Now().Add(CaptchaDuration)
					}

					infoLogger.Printf(
						"Captcha for %s will turn off after %ds\n",
						Config[i].Service, int64(time.Until(Config[i].CaptchaTill).Seconds()),
					)
				}
			}
		}

		hasPreviousRPS = true
		previousRPS = serviceRPS
		prevEnd = time.Now()
	}
}

func PrintEndpoints() {
	endpoints := ResolveAllEndpoints()
	for i := range endpoints {
		fmt.Println(endpoints[i].FQDN)
	}
}

func ParseArgs() {
	var timeout int
	var captchaDuration int
	flag.StringVar(&StablePODSET, "stable", "prod-antirobot-yp-", "Specify prefix of stable PODSET. Default is prod-antirobot-yp-")
	flag.StringVar(&PrestablePODSET, "prestable", "prod-antirobot-yp-prestable-", "Specify prefix of prestable PODSET. Default is prod-antirobot-yp-prestable-")
	flag.IntVar(&captchaDuration, "captcha", 300, "Specify captcha duration. Default value is 300s")
	flag.IntVar(&timeout, "timeout", 10, "Specify timeout for its requests. Default value is 10s")
	CaptchaDuration = time.Duration(captchaDuration) * time.Second
	Timeout = time.Duration(timeout) * time.Second
}

func main() {
	ParseArgs()
	InitLogger()
	LoadNannyToken()
	infoLogger.Println("script was started")
	RunDetector()
	//PrintEndpoints()
}
