package main

import (
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"bytes"
	"context"
	"crypto/tls"
	"encoding/json"
	"errors"
	"fmt"
	"gopkg.in/yaml.v2"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"time"
)

type Config struct {
	SolomonURL     string        `yaml:"SolomonURL"`
	SolomonProject string        `yaml:"SolomonProject"`
	SolomonCluster string        `yaml:"SolomonCluster"`
	SolomonService string        `yaml:"SolomonService"`
	ClientTimeout  time.Duration `yaml:"ClientTimeout"`
	TestZone       string        `yaml:"TestZone"`
}

// iam token from metadata, GCE format
type IamToken struct {
	ExpiresIn   int    `json:"expires_in" validate:"required"`
	AccessToken string `json:"access_token" validate:"required"`
	TokenType   string `json:"token_type"`
}

const (
	ConfigPath  = `C:\YandexCloud\kms-e2e-config.yml`
	iamTokenURL = `http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token`
)

func CheckClose(c io.Closer, err *error) {
	cErr := c.Close()
	if *err == nil {
		*err = cErr
	}
}

func makeSolomonURL(cfg Config) (url string) {
	return fmt.Sprintf(`https://%s/api/v2/push?project=%s&cluster=%s&service=%s`, cfg.SolomonURL,
		cfg.SolomonProject, cfg.SolomonCluster, cfg.SolomonService)
}

func is2xxResponse(resp *http.Response) bool {
	return resp.StatusCode >= 200 && resp.StatusCode < 300
}

func isPersistent(resp *http.Response) bool {
	return resp.StatusCode < 500 || resp.StatusCode >= 600
}

func getIamToken(cfg Config) (string, error) {
	var (
		data   bytes.Buffer
		iToken IamToken
	)

	client := http.Client{
		Timeout: cfg.ClientTimeout * time.Second,
	}

	request, err := http.NewRequest("GET", iamTokenURL, &data)
	if err != nil {
		return "", err
	}

	request.Header.Add("Metadata-Flavor", "Google")
	resp, err := client.Do(request)
	if err != nil {
		return "", err
	}

	if resp != nil {
		log.Print(resp.StatusCode)
		log.Print(resp.Body)
		log.Print(resp)
	}

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}
	defer CheckClose(resp.Body, &err)

	err = json.Unmarshal(body, &iToken)
	if err != nil {
		return "", err
	}

	return iToken.AccessToken, nil
}

func sendMetric(data *bytes.Buffer, cfg Config, token string) (err error) {
	var (
		resp          *http.Response
		retryCount    = 5
		retryDelay    = time.Duration(5) * time.Second
		MaxRetryCount = 5
	)

	tlsConfig := &tls.Config{
		InsecureSkipVerify: true,
	}
	tr := &http.Transport{
		TLSClientConfig:       tlsConfig,
		ResponseHeaderTimeout: time.Second * 10,
	}

	client := http.Client{
		Timeout:   cfg.ClientTimeout * time.Second,
		Transport: tr,
	}

	log.Print("Request data: " + data.String())

	ctx := context.Background()
	req, _ := http.NewRequestWithContext(ctx, "POST", makeSolomonURL(cfg), data)

	req.Header.Add("Authorization", "Bearer "+token)
	req.Header.Set("Content-Type", "application/json")

	for retryCount > 0 {
		if retryCount != MaxRetryCount {
			log.Printf("Try #%d %s\n", MaxRetryCount-retryCount, req.URL.String())
		}

		resp, err = client.Do(req)
		if err != nil {
			log.Print(req)
			if resp != nil {
				log.Print(resp.StatusCode)
				log.Print(resp.Body)
				log.Print(resp)
			}
			return err
		}

		if is2xxResponse(resp) {
			break
		} else if isPersistent(resp) {
			break
		} else {
			retryCount--
			time.Sleep(retryDelay)
		}
	}

	log.Print(resp.StatusCode)
	log.Print(resp.Body)
	log.Print(resp)
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return err
	}
	defer CheckClose(resp.Body, &err)
	log.Print("Response" + string(body))

	if !is2xxResponse(resp) {
		return errors.New("error sending metrics to solomon")
	}

	return nil
}

func readConfig(cfg *Config, path string) (err error) {
	f, err := os.Open(path)
	if err != nil {
		return err
	}

	defer CheckClose(f, &err)

	decoder := yaml.NewDecoder(f)
	err = decoder.Decode(cfg)
	if err != nil {
		return err
	}
	return nil
}

func checkKms() (result float64, err error) {
	var out bytes.Buffer
	var stderr bytes.Buffer

	cmd := exec.Command(`cmd.exe`, `/C`, `C:\Windows\System32\cscript.exe`, `C:\Windows\System32\slmgr.vbs`, `/ato`)

	cmd.Stdout = &out
	cmd.Stderr = &stderr
	err = cmd.Run()

	//slmgr.vbs returns non-zero exit code on failed activation
	//more info here: https://docs.microsoft.com/en-us/windows-server/get-started/activation-error-codes
	if err != nil {
		log.Print("stdout: " + out.String())
		log.Print("stderr: " + stderr.String())
		return 0, err
	} else {
		log.Print("Activated Successfully")
		log.Printf("Got text: %s\n", out.String())
		return 1, nil
	}
}

func prepareData(host string, kmsRegistry solomon.Registry, kmsAvailable metrics.Gauge) (data []byte, err error) {
	var buf bytes.Buffer
	_, err = kmsRegistry.StreamJSON(context.Background(), &buf)
	if err != nil {
		return nil, err
	}

	// we need to add host field into metric
	// because arcadia lib doesn't have this field in stream json
	var m map[string]interface{}
	err = json.Unmarshal(buf.Bytes(), &m)
	if err != nil {
		return nil, err
	}
	m["commonLabels"] = map[string]string{"host": host}
	newData, err := json.Marshal(m)
	if err != nil {
		return nil, err
	}

	return newData, nil
}

func main() {
	var cfg Config

	err := readConfig(&cfg, ConfigPath)
	if err != nil {
		log.Fatal(err.Error())
	}

	regOpts := solomon.RegistryOpts{
		Tags: map[string]string{"name": cfg.TestZone},
	}
	kmsRegistry := solomon.NewRegistry(&regOpts)
	kmsAvailable := kmsRegistry.Gauge("kms_available")

	log.Print("Starting e2e kms key test")
	ticker := time.NewTicker(1 * time.Minute)

	for range ticker.C {
		res, err := checkKms()
		if err != nil {
			//vbs script error should not be fatal
			log.Print(err.Error())
		}

		kmsAvailable.Set(res)
		data, err := prepareData(cfg.TestZone, *kmsRegistry, kmsAvailable)
		if err != nil {
			log.Fatal(err.Error())
		}

		token, err := getIamToken(cfg)
		if err != nil {
			log.Fatal(err.Error())
		}

		err = sendMetric(bytes.NewBuffer(data), cfg, token)
		if err != nil {
			log.Fatal(err.Error())
		}
	}
}
