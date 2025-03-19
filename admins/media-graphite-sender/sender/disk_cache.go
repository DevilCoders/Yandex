package sender

import (
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"sync"
	"time"

	"a.yandex-team.ru/library/go/core/log"
)

const timeForm = "20060102-15:04:05"

// addToDiskCache saves metric to disk
func (s *Sender) addToDiskCache(buf *senderBuffer, server string) error {
	defer metricsDuration("spool."+server, time.Now())

	serverDir := path.Join(s.config.Sender.QueueDir, server)
	filePath := path.Join(serverDir, time.Now().Format(timeForm) /* fileName */)

	if _, err := os.Stat(serverDir); err != nil {
		if !os.IsNotExist(err) {
			return fmt.Errorf("invalid cache dir %s: %w", serverDir, err)
		}
		if err := os.MkdirAll(serverDir, 0700); err != nil {
			return fmt.Errorf("failed to create cache dir %s: %w", serverDir, err)
		}
	}

	file, err := os.OpenFile(filePath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0640)
	defer func() { _ = file.Close() }()
	if err != nil {
		return fmt.Errorf("failed to open cache %s: %w", filePath, err)
	}
	logger.Debugf("Write spool data file=%s, size=%d", filePath, buf.size())
	if _, err := file.Write(buf.bytes()); err != nil {
		return fmt.Errorf("failed with cache file %s: %w", file.Name(), err)
	}
	return nil
}

// saveBuffersAtExit on exit sigals save buf and delayedBuffer to disk cache
func (s *Sender) saveBuffersAtExit(buf *senderBuffer, delayed [][]byte) {
	if buf.len() == 0 && len(delayed) == 0 {
		return
	}

	for _, metric := range delayed {
		if logTrace {
			logger.Debugf("Save delayed metric to disk metric=%q", string(metric))
		}
		buf.add(metric)
	}
	logger.Infof("Save buffers to disk cache metrics=%d, size=%d", buf.len(), buf.size())
	graphiteServers := s.config.Graphite.Servers
	for _, srv := range graphiteServers {
		if err := s.addToDiskCache(buf, srv); err != nil {
			logger.Errorf("Failed to save data at exit: %v", err)
		}
	}
}

// sendCachedData send cached data from disk
func (s *Sender) sendCachedData() {
	var wg sync.WaitGroup

	queueDir := s.config.Sender.QueueDir
	logger.Debugf("Send cache interval=%s, speedup=%d",
		s.config.Sender.SendInterval/time.Duration(s.config.Sender.CacheSpeedup),
		s.config.Sender.CacheSpeedup,
	)

	if _, err := os.Stat(queueDir); err != nil && os.IsNotExist(err) {
		if os.IsNotExist(err) {
			if err := os.MkdirAll(queueDir, 0700); err != nil {
				logger.Fatalf("Cannot create cache dir=%s: %v", queueDir, err)
			}
		} else {
			logger.Fatalf("Invalid cache dir=%s: %v", queueDir, err)
		}
	}
	for {
		dirs, err := ioutil.ReadDir(queueDir)
		if err != nil {
			logger.Fatalf("Cannot open queue dir: %v", err)
		}
		for _, dir := range dirs {
			name := dir.Name()
			if !dir.IsDir() {
				logger.Errorf("Not a directory in queue file=%s", name)
			}
			wg.Add(1)
			go func(server string, w *sync.WaitGroup) {
				defer w.Done()
				files, _ := ioutil.ReadDir(path.Join(queueDir, server))
				for _, file := range files {
					fileName := path.Join(queueDir, server, file.Name())
					s.sendFileFromCache(server, fileName)
				}
			}(name, &wg)
		}
		wg.Wait()
		time.Sleep(s.config.Sender.SendInterval)
	}
}

func (s *Sender) sendFileFromCache(server, fileName string) {
	sendInterval := s.config.Sender.SendInterval
	speedup := s.config.Sender.CacheSpeedup
	cacheInterval := sendInterval / time.Duration(speedup)
	logCache := log.With(logger, log.String("cache", fileName), log.String("server", server))

	fileTime, err := time.ParseInLocation(timeForm, fileName, time.Local)
	if err != nil {
		logCache.Warn("Invalid cache filename, remove it.")
		cleanCache(fileName)
		return
	}
	// Send cache that is older then send_interval seconds
	// Prevent race condition if new cache file is being written
	if time.Since(fileTime) < sendInterval {
		logCache.Debug("Skip too fresh cache")
		return
	}
	if time.Since(fileTime) > s.config.Sender.QueueTimeout {
		logCache.Warn("Cache expired, remove it")
		cleanCache(fileName)
		return
	}

	buf, err := s.store.bufferFromFile(fileName)
	defer s.store.returnToQueue(buf)
	if err != nil {
		logCache.Errorf("Read cache err, try clean: %v", err)
		cleanCache(fileName)
		return
	}
	gConn := s.createGraphiteConn(server, cacheInterval)
	if gConn == nil {
		return
	}

	logCache.Infof("Sending cached (disk) metrics=%d, size=%d", buf.len(), buf.size())
	if err := sendToGraphite(server, gConn, buf); err != nil {
		logCache.Warn("Failed to sending cache (disk)")
		return
	}
	logCache.Info("Clean disk cache")
	cleanCache(fileName)
	time.Sleep(cacheInterval)
}

func cleanCache(fileName string) {
	if err := os.Remove(fileName); err != nil {
		logger.Errorf("Clean failed file=%s: %v", fileName, err)
	}
}
