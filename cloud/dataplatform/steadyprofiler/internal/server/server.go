package server

import (
	"fmt"
	"io/ioutil"
	"os/exec"
	"strconv"
	"time"

	"github.com/araddon/dateparse"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/config"
	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/store"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/transfer_manager/go/pkg/format"
)

func Start(cfg *config.Config) error {
	stor, err := store.NewStore(cfg)
	if err != nil {
		logger.Log.Error("Unable to start echo server", log.Error(err))
		return err
	}
	// Echo instance
	todayServices, err := stor.ListServices(time.Now())
	if err != nil {
		logger.Log.Error("Unable to read today services", log.Error(err))
		return err
	}
	go func() {
		for {
			st := time.Now()
			t, err := stor.ListServices(time.Now())
			if err != nil {
				logger.Log.Error("Unable to read today services", log.Error(err))
			} else {
				logger.Log.Infof("services loaded with %v in %v", len(t.Services), time.Since(st))
				todayServices = t
			}
			time.Sleep(time.Minute)
		}
	}()
	e := echo.New()
	e.Use(func(next echo.HandlerFunc) echo.HandlerFunc {
		return func(c echo.Context) (err error) {
			req := c.Request()
			start := time.Now()
			logger.Log.Infof("start [%v]:[%v]", req.Method, req.URL.String())
			err = next(c)
			res := c.Response()
			if err == nil {
				logger.Log.Infof("done [%v]:[%v] in %v res: %v", req.Method, req.URL.String(), time.Since(start), format.SizeUInt64(uint64(res.Size)))
			} else {
				logger.Log.Warnf("done [%v]:[%v] in %v res: %v: %v", req.Method, req.URL.String(), time.Since(start), format.SizeUInt64(uint64(res.Size)), err)
			}
			return err
		}
	})
	e.Use(middleware.Recover())
	e.GET("/", func(c echo.Context) error {
		ts := c.QueryParam("from_ts")
		parsedTS, err := dateparse.ParseAny(ts)
		if err != nil {
			parsedTS = time.Now()
		}
		date := parsedTS.Format("2006-01-02")
		res := todayServices
		if date != time.Now().Format("2006-01-02") || len(todayServices.Services) == 0 {
			res, err = stor.ListServices(parsedTS)
			if err != nil {
				logger.Log.Warn("err", log.Error(err))
			}
		}
		return c.JSON(200, res)
	})
	e.GET("/service/:service/types", func(c echo.Context) error {
		// TODO: Detect service collected types
		return c.JSON(200, []string{
			"unknown",
			"cpu",
			"heap",
			"block",
			"mutex",
			"goroutine",
			"threadcreate",
			"other",
			"trace",
		})
	})
	e.GET("/service/:service/type/:type/resource/:resource_id/profiles", func(c echo.Context) error {
		service := c.Param("service")
		resourceID := c.Param("resource_id")
		typ := c.Param("type")
		minTS := c.QueryParam("min_ts")
		maxTS := c.QueryParam("max_ts")
		parsedMinTS, err := dateparse.ParseAny(minTS)
		if err != nil {
			logger.Log.Warn("unable to parse TS", log.Error(err))
			parsedMinTS = time.Now().Add(-time.Hour)
		}
		parsedMaxTS, err := dateparse.ParseAny(maxTS)
		if err != nil {
			logger.Log.Warn("unable to parse TS", log.Error(err))
			parsedMaxTS = time.Now()
		}
		res, err := stor.ListProfiles(parsedMinTS, parsedMaxTS, service, resourceID, typ)
		if err != nil {
			return err
		}
		return c.JSON(200, res)
	})
	e.GET("/service/:service/type/:type/resource/:resource_id/profile/:ts", func(c echo.Context) error {
		service := c.Param("service")
		profileType := c.Param("type")
		resourceID := c.Param("resource_id")
		ts := c.Param("ts")
		tsNano, _ := strconv.ParseInt(ts, 10, 64)
		data, err := stor.ProfileBlob(time.Unix(0, tsNano), service, profileType, resourceID)
		if err != nil {
			return c.NoContent(404)
		}
		return c.Blob(200, "application/octet-stream", data)
	})
	e.GET("/service/:service/type/:type/resource/:resource_id/pprof_result/:ts/:out_type", func(c echo.Context) error {
		service := c.Param("service")
		profileType := c.Param("type")
		resourceID := c.Param("resource_id")
		outTyp := c.Param("out_type")
		ts := c.Param("ts")
		tsNano, _ := strconv.ParseInt(ts, 10, 64)
		data, err := stor.ProfileBlob(time.Unix(0, tsNano), service, profileType, resourceID)
		if err != nil {
			return c.NoContent(404)
		}
		file, err := ioutil.TempFile("", "profile")
		if err != nil {
			return c.String(500, err.Error())
		}
		if _, err := file.Write(data); err != nil {
			return c.String(500, err.Error())
		}
		cmd := exec.Command("ya", "tool", "go", "tool", "pprof", fmt.Sprintf("-%v", outTyp), file.Name())
		logger.Log.Infof("cmd: %v", cmd.Args)
		out, err := cmd.CombinedOutput()
		if err != nil {
			logger.Log.Error("unable to exec command", log.Error(err))
			return c.String(500, fmt.Sprintf("%v: %v", err.Error(), string(out)))
		}
		return c.String(200, string(out))
	})

	return e.Start(cfg.HTTPPort)
}
