package uatraits

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os/exec"
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/test/yatest"
)

func createTraits(data map[string]interface{}) Traits {
	traits := newTraits()
	for k, v := range data {
		switch vt := v.(type) {
		case string:
			traits[k] = vt
		case bool:
			if vt {
				traits[k] = "true"
			} else {
				traits[k] = "false"
			}
		}
	}
	return traits
}

func createDetector(options ...func(*Detector)) (*Detector, error) {
	return NewDetector(options...)
}

func generateTestData(outputPath string) error {
	testHelperBinaryPath := yatest.BuildPath("library/go/yandex/uatraits/test_generator/test_generator")
	userAgentsListFilePath := yatest.WorkPath("user_agents.json")

	command := exec.Command(testHelperBinaryPath, "--input", userAgentsListFilePath, "--output", outputPath)
	output, err := command.CombinedOutput()
	if err != nil {
		return xerrors.Errorf("test_helper returned non-zero status with output: %s", output)
	} else {
		return nil
	}
}

func TestDetect(t *testing.T) {
	detector, err := createDetector()
	if !assert.NoError(t, err) {
		assert.FailNowf(t, "cannot create detector", "%w", err)
	}

	userAgentsTestsFilePath := yatest.WorkPath("user_agents_tests.json")

	// Run test helper script to generate etalon fields.
	if err := generateTestData(userAgentsTestsFilePath); err != nil {
		assert.FailNowf(t, "cannot generate test data", "%w", err)
	}

	userAgentBytes, err := ioutil.ReadFile(userAgentsTestsFilePath)
	if err != nil {
		assert.FailNowf(t, "cannot load test data", "%w", err)
	}
	var userAgentsData []struct {
		UserAgent string                 `json:"user_agent"`
		Traits    map[string]interface{} `json:"traits"`
	}

	if err := json.Unmarshal(userAgentBytes, &userAgentsData); err != nil {
		assert.FailNow(t, "cannot load user-agent list", err)
	}

	for i, item := range userAgentsData {
		actualTraits := detector.Detect(item.UserAgent)
		expectedTraits := createTraits(item.Traits)
		assert.Equal(t, expectedTraits, actualTraits, "Test %d / %d: \"%s\"", i+1, len(userAgentsData), item.UserAgent)
	}
}

func createHTTPHeaders(headers map[string]string) http.Header {
	httpHeaders := make(http.Header)
	for k, v := range headers {
		httpHeaders.Set(k, v)
	}
	return httpHeaders
}

func TestDetectByHeaders(t *testing.T) {
	// To disable caching we need to create separate instances of detector
	getDetector := func() *Detector {
		detector, err := createDetector()
		if !assert.NoError(t, err) {
			assert.FailNowf(t, "cannot create detector", "%w", err)
		}
		return detector
	}

	t.Run("X-Operamini-Phone-Ua", func(t *testing.T) {
		userAgent := "Opera/9.80 (X11; Linux zvav; U; ru) Presto/2.12.423 Version/12.16"
		xOperaMiniPhoneUa := "NokiaX2-00/5.0 (04.80) Profile/MIDP-2.1 Configuration/CLDC-1.1 Mozilla/5.0 AppleWebKit/420+ (KHTML, like Gecko) Safari/420+"
		actualTraits := getDetector().DetectByHeaders(createHTTPHeaders(map[string]string{
			"User-Agent":           userAgent,
			"X-Operamini-Phone-Ua": xOperaMiniPhoneUa,
		}))

		expectedTraits := getDetector().Detect(userAgent)
		for k, v := range map[string]string{
			"OSFamily":     "Symbian",
			"DeviceModel":  "X2-00",
			"DeviceVendor": "Nokia",
			"isMobile":     "true",
		} {
			expectedTraits.Set(k, v)
		}
		assert.Equal(t, expectedTraits, actualTraits)
	})

	t.Run("X-Wap-Profile", func(t *testing.T) {
		userAgent := "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.12 Safari/537.36"
		actualTraits := getDetector().DetectByHeaders(createHTTPHeaders(map[string]string{
			"User-Agent":    userAgent,
			"X-Wap-Profile": "http://122.200.68.229/docs/mini3ix.xml",
		}))

		expectedTraits := getDetector().Detect(userAgent)
		// See the first profile in profiles.xml

		expectedTraits.Set("DeviceKeyboard", "SoftKeyPad")
		expectedTraits.Set("DeviceModel", "Dell_Mini3iX")
		expectedTraits.Set("DeviceVendor", "dell")
		expectedTraits.Set("ScreenSize", "360x640")
		expectedTraits.Set("BitsPerPixel", "24")
		expectedTraits.Set("ScreenWidth", "360")
		expectedTraits.Set("ScreenHeight", "640")

		assert.Equal(t, expectedTraits, actualTraits)
	})

	t.Run("AdditionalHeader", func(t *testing.T) {
		userAgents := []string{
			"Mozilla/5.0 (Web0S; Linux/SmartTV) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/38.0.2125.122 Safari/537.36 WebAppManager",
			"Mozilla/5.0 (SMART-TV; LINUX; Tizen 3.0) AppleWebKit/538.1 (KHTML, like Gecko) Version/3.0 TV Safari/538.1",
			"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36",
			"Mozilla/5.0 (SMART-TV; Linux; Tizen 2.4.0) AppleWebKit/538.1 (KHTML, like Gecko) Version/2.4.0 TV Safari/538.1",
			"Mozilla/5.0 (SmartHub; SMART-TV; U; Linux/SmartTV+2014; Maple2012) AppleWebKit/537.42+ (KHTML, like Gecko) SmartTV Safari/537.42+",
		}
		additionalHeaders := []string{
			"Mozilla/5.0 (Linux; arm_64; Android 10; SM-A505FM) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 BroPP/1.0 SA/1 Mobile Safari/537.36 YandexSearch/11.30",
			"Mozilla/5.0 (Linux; arm_64; Android 10; STK-LX1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 BroPP/1.0 SA/1 Mobile Safari/537.36 YandexSearch/11.30",
			"Mozilla/5.0 (Linux; arm; Android 7.1.1; SM-J510FN) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 BroPP/1.0 SA/1 Mobile Safari/537.36 YandexSearch/11.30",
			"Mozilla/5.0 (Linux; arm_64; Android 10; SM-A505FN) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 BroPP/1.0 SA/1 Mobile Safari/537.36 YandexSearch/11.40",
			"Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.116 YaBrowser/20.7.2.124 Yowser/2.5 Safari/537.36",
		}
		for _, userAgent := range userAgents {
			for _, additionalHeader := range additionalHeaders {
				expectedTraits := getDetector().Detect(userAgent)
				// Additional user agent should override some of the fields
				additionalAgentFullTraits := getDetector().Detect(additionalHeader)
				for _, field := range fieldsForAdditionalHeaders {
					if value := additionalAgentFullTraits.Get(field); value != "" {
						expectedTraits.Set(field, value)
					}
				}

				actualTraits := getDetector().DetectByHeaders(createHTTPHeaders(map[string]string{
					"User-Agent":      userAgent,
					"Device-Stock-UA": additionalHeader,
				}))

				assert.Equal(t, expectedTraits, actualTraits)
			}
		}
	})
}

func TestCacheHitMetrics(t *testing.T) {
	testCases := map[string]struct {
		userAgents          []string
		expectedHitCounter  int
		expectedMissCounter int
	}{
		"one userAgent": {
			userAgents: []string{
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
			},
			expectedHitCounter:  4,
			expectedMissCounter: 1,
		},
		"all different userAgents": {
			userAgents: []string{
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.2 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.3 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.4 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.5 Safari/537.36",
			},
			expectedHitCounter:  0,
			expectedMissCounter: 5,
		},
		"combined data": {
			userAgents: []string{
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.2 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.2 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.3 Safari/537.36",
			},
			expectedHitCounter:  2,
			expectedMissCounter: 3,
		},
		"combined data 2": {
			userAgents: []string{
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.2 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.2 Safari/537.36",
			},
			expectedHitCounter:  3,
			expectedMissCounter: 2,
		},
		"combined data 3": {
			userAgents: []string{
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.1 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.2 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.3 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.3 Safari/537.36",
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.2 Safari/537.36",
			},
			expectedHitCounter:  2,
			expectedMissCounter: 3,
		},
	}

	for testName, testCase := range testCases {
		testName, testCase := testName, testCase

		mockMetrics := newMockCacheHitMetrics()
		detector, err := createDetector(EnableMetrics(mockMetrics))
		if !assert.NoError(t, err) {
			assert.FailNowf(t, "cannot create detector", "%w", err)
		}

		t.Run(testName, func(t *testing.T) {
			t.Parallel()

			for _, userAgent := range testCase.userAgents {
				_ = detector.Detect(userAgent)
			}

			assert.Equal(t, testCase.expectedHitCounter, mockMetrics.getHitCounter())
			assert.Equal(t, testCase.expectedMissCounter, mockMetrics.getMissCounter())
		})
	}
}

type mockCacheHitMetrics struct {
	mutex       sync.RWMutex
	hitCounter  int
	missCounter int
}

func newMockCacheHitMetrics() *mockCacheHitMetrics {
	return &mockCacheHitMetrics{
		mutex:       sync.RWMutex{},
		hitCounter:  0,
		missCounter: 0,
	}
}

func (t *mockCacheHitMetrics) HitIncrement() {
	t.mutex.Lock()
	defer t.mutex.Unlock()
	t.hitCounter++
}

func (t *mockCacheHitMetrics) getHitCounter() int {
	t.mutex.RLock()
	defer t.mutex.RUnlock()
	return t.hitCounter
}

func (t *mockCacheHitMetrics) MissIncrement() {
	t.mutex.Lock()
	defer t.mutex.Unlock()
	t.missCounter++
}

func (t *mockCacheHitMetrics) getMissCounter() int {
	t.mutex.RLock()
	defer t.mutex.RUnlock()
	return t.missCounter
}

func BenchmarkDetector(b *testing.B) {
	detector, err := createDetector()
	if err != nil {
		b.FailNow()
	}

	b.Run("FirstHit", func(b *testing.B) {
		userAgentTemplate := "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.%d Safari/537.36"
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			b.StopTimer()
			// explicitly generate unique User-Agent to be sure that we miss the cache
			userAgent := fmt.Sprintf(userAgentTemplate, i)
			b.StartTimer()
			_ = detector.Detect(userAgent)
		}
	})

	b.Run("CachedHit", func(b *testing.B) {
		userAgent := "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4147.105 Safari/537.36"
		// Warm-up cache
		_ = detector.Detect(userAgent)
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = detector.Detect(userAgent)
		}
	})
}
