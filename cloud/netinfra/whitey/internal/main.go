package internal

import (
	"log"
	"sync"
	"time"
	"whitey/collector"
)

type App struct {
	Independent bool
	Interval    time.Duration
	Collectors  []collector.Collector
}

func (a *App) Run() {
	if a.Independent {
		for _, col := range a.Collectors {
			go func(c collector.Collector) {
				ticker := time.NewTicker(c.GetInterval())

				for range ticker.C {
					err := c.Collect()
					if err != nil {
						log.Fatal(err)
					}
					// place to log data collection results

					for _, t := range c.GetTasks() {
						t.GetMetrics()
						// place to log metrics parsing results

						err := t.Push()
						if err != nil {
							log.Fatal(err)
						}
						// place to log metrics push results
					}

					c.Drop()
				}
			}(col)
		}
		select {}
	} else {
		wg := sync.WaitGroup{}

		collectInterval := a.Interval * time.Second

		ticker := time.NewTicker(collectInterval)
		stop := make(chan bool)

		for {
			select {
			case <-stop:
				ticker.Stop()
				return
			case <-ticker.C:
				wg.Add(len(a.Collectors))
				for _, col := range a.Collectors {
					go func(c collector.Collector) {
						err := c.Collect()
						if err != nil {
							log.Fatal(err)
						}
						// place to log metrics colletion errors

						for _, t := range c.GetTasks() {
							err := t.GetMetrics()
							if err != nil {
								log.Fatal(err)
							}
							// place to log metrics parsing errors

							err = t.Push()
							if err != nil {
								log.Fatal(err)
							}
							// place to log metrics push errors
						}

						c.Drop()

						wg.Done()
					}(col)
				}
				wg.Wait()
			}
		}
	}
}
