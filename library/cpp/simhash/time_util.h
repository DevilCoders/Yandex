#pragma once

#include <util/datetime/systime.h>
#include <util/generic/yexception.h>
#include <util/generic/noncopyable.h>
#include <util/system/defaults.h>

class TTimer: private TNonCopyable {
public:
    TTimer()
        : start()
        , stop()
        , started(false)
        , elapsed()
    {
        Reset();
    }

public:
    void Start() {
        if (started) {
            ythrow yexception() << "TTimer::Start: You need to stop the timer";
        }
        gettimeofday(&start, nullptr);
        started = true;
    }

    void Stop() {
        if (!started) {
            ythrow yexception() << "TTimer::Start: You need to start the timer";
        }
        gettimeofday(&stop, nullptr);
        started = false;

        elapsed.tv_sec += stop.tv_sec - start.tv_sec;
        elapsed.tv_usec += stop.tv_usec - start.tv_usec;
    }

    void Reset() {
        start.tv_sec = 0;
        start.tv_usec = 0;
        stop.tv_sec = 0;
        stop.tv_usec = 0;
        started = false;
        elapsed.tv_sec = 0;
        elapsed.tv_usec = 0;
    }

    timeval Elapsed() const {
        if (started) {
            ythrow yexception() << "TTimer::Elapsed: You need to stop the timer";
        }
        return elapsed;
    }

    ui64 SecondsElapsed() const {
        if (started) {
            ythrow yexception() << "TTimer::SecondsElapsed: You need to stop the timer";
        }
        return elapsed.tv_sec + elapsed.tv_usec / 1000000;
    }

    ui64 MillisecondsElapsed() const {
        if (started) {
            ythrow yexception() << "TTimer::MillisecondsElapsed: You need to stop the timer";
        }
        return elapsed.tv_sec * 1000 + elapsed.tv_usec / 1000;
    }

    ui64 MicrosecondsElapsed() const {
        if (started) {
            ythrow yexception() << "TTimer::MicrosecondsElapsed: You need to stop the timer";
        }
        return elapsed.tv_sec * 1000000 + elapsed.tv_usec;
    }

private:
    timeval start;
    timeval stop;
    bool started;
    timeval elapsed;
};
