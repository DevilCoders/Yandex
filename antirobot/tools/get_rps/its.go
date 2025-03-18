package main

import (
	"a.yandex-team.ru/library/go/certifi"
	"bytes"
	"crypto/tls"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"strings"
	"time"
)

type ITSService struct {
	Etag        map[string]string
	CaptchaTill time.Time
}

func (s ITSService) AllPressed() bool {
	if len(s.Etag) == 0 {
		return false
	}

	for _, etag := range s.Etag {
		if len(etag) == 0 {
			return false
		}
	}
	return true
}

type ITS struct {
	NannyToken      string
	ITSRuchkas      map[string]string
	CTypes          []string
	Config          map[string]*ITSService
	CaptchaDuration time.Duration
	Client          http.Client
}

func NewITS(filename string, captchaDuration time.Duration, services map[string]bool, itsRuchkas map[string]string) (*ITS, error) {
	var its = ITS{
		ITSRuchkas:      itsRuchkas,
		CTypes:          []string{"stable", "prestable"},
		CaptchaDuration: captchaDuration,
	}

	its.Config = make(map[string]*ITSService)

	for s := range services {
		its.Config[s] = &ITSService{
			Etag:        make(map[string]string),
			CaptchaTill: time.Now(),
		}
	}

	if err := its.loadNannyToken(filename); err != nil {
		return nil, err
	}

	its.Client = createClient()

	return &its, nil
}

func createClient() http.Client {
	tlsConfig := &tls.Config{}
	certPool, err := certifi.NewCertPool()
	if err != nil {
		log.Fatalf("failed to create cert pool: %v\n", err)
	} else {
		tlsConfig.RootCAs = certPool
	}

	return http.Client{
		Transport: &http.Transport{
			TLSClientConfig: tlsConfig,
		},
	}
}

func (its *ITS) loadNannyToken(filename string) error {
	f, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer func() {
		if err = f.Close(); err != nil {
			log.Fatal(err)
		}
	}()

	content, err := ioutil.ReadAll(f)
	if err != nil {
		return err
	}

	its.NannyToken = strings.TrimSpace(string(content))
	return nil
}

func (its *ITS) Enable(service string, ruchkaName string) {
	for _, ctype := range its.CTypes {
		if ruchka, ok := its.ITSRuchkas[ruchkaName]; ok {
			r, resp, status := its.enable(service, ctype, ruchka)
			log.Println("ITS enable:", service, ruchka, ctype, status, resp)

			if len(r) == 0 {
				break
			} else {
				its.Config[service].Etag[ruchka+ctype] = r
				its.Config[service].CaptchaTill = time.Now().Add(its.CaptchaDuration)
			}
		}
	}
}

func (its *ITS) Disable(service string, ruchkaName string) {
	for _, ctype := range its.CTypes {
		if ruchka, ok := its.ITSRuchkas[ruchkaName]; ok {
			resp, status := its.disable(service, its.Config[service].Etag[ruchka+ctype], ctype, ruchka)
			log.Println("ITS disable:", service, ruchka, ctype, status, resp)
			its.Config[service].Etag[ruchka+ctype] = ""
		}
	}
}

func getURL(service string, ctype string, ruchka string) string {
	return fmt.Sprintf("https://its.yandex-team.ru/v1/values/antirobot/antirobot/%s/%s/%s/", service, ctype, fmt.Sprintf(ruchka, service))
}

func (its *ITS) enable(service string, ctype string, ruchka string) (etag string, content string, statusCode int) {
	itsURL := getURL(service, ctype, ruchka)

	req, _ := http.NewRequest("POST", itsURL, bytes.NewBuffer([]byte(`{"value":"`+service+`"}`)))
	req.Header.Set("Authorization", "Oauth "+its.NannyToken)
	req.Header.Set("Content-Type", "text/plain")

	resp, err := its.Client.Do(req)

	if err != nil {
		log.Printf("EnableITS failed: %v", err)
		return
	}

	etag = resp.Header.Get("ETag")
	body, err := ioutil.ReadAll(resp.Body)
	defer resp.Body.Close()
	if err != nil {
		log.Fatal(err)
	}

	content = string(body)
	statusCode = resp.StatusCode
	return
}

func (its *ITS) disable(service string, etag string, ctype string, ruchka string) (content string, statusCode int) {
	itsURL := getURL(service, ctype, ruchka)

	req, _ := http.NewRequest(http.MethodDelete, itsURL, nil)
	req.Header.Set("Authorization", "Oauth "+its.NannyToken)
	req.Header.Set("Content-Type", "text/plain")
	req.Header.Set("If-Match", etag)

	resp, err := its.Client.Do(req)
	if err != nil {
		log.Fatal(err)
	}

	body, err := ioutil.ReadAll(resp.Body)
	defer resp.Body.Close()
	if err != nil {
		log.Fatal(err)
	}

	content = string(body)
	statusCode = resp.StatusCode
	return
}
