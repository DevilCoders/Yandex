#pragma once

extern "C" {
#include <ngx_core.h>
}

#include <util/generic/yexception.h>
#include <util/string/builder.h>

namespace NStrm::NPackager {
    class TNgxStreamLogger {
    public:
        TNgxStreamLogger(const ngx_uint_t level, ngx_log_t* log)
            : Level_(level)
            , Log_(log)
        {
            Y_ENSURE(Log_);
        }

        TNgxStreamLogger(TNgxStreamLogger&& logger)
            : Level_(logger.Level_)
            , Log_(logger.Log_)
            , Str_(std::move(logger.Str_))
        {
        }

        TNgxStreamLogger(const TNgxStreamLogger&) = delete;

        ~TNgxStreamLogger() {
            if (!Str_.Empty()) {
                ngx_log_error(Level_, Log_, 0, "%s", Str_.Data());
            }
        }

        template <typename T>
        TNgxStreamLogger& operator<<(const T& t) {
            if (Log_->log_level < Level_) {
                return *this;
            }

            Str_ << t;
            return *this;
        }

        TNgxStreamLogger& operator<<(const ngx_str_t& s) {
            if (Log_->log_level < Level_) {
                return *this;
            }

            Str_ << TStringBuf((char*)s.data, s.len);
            return *this;
        }

        TNgxStreamLogger& operator<<(const std::exception_ptr& e) {
            if (Log_->log_level < Level_) {
                return *this;
            }

            try {
                std::rethrow_exception(e);
            } catch (const yexception& ye) {
                Str_ << "c++ exception [[ " << ye.what() << " ]] ";
                if (ye.BackTrace()) {
                    Str_ << "backtrace [[ " << ye.BackTrace()->PrintToString() << " ]] ";
                }
            } catch (const std::exception& se) {
                Str_ << "c++ exception [[ " << se.what() << " ]] ";
            } catch (...) {
                Str_ << "c++ unknown exception";
            }
            return *this;
        }

    private:
        const ngx_uint_t Level_;
        ngx_log_t* const Log_;
        TStringBuilder Str_;
    };

    class TNgxLogger {
    public:
        TNgxLogger(const TNgxLogger&) = default;

        TNgxLogger(const ngx_uint_t level, ngx_log_t* log)
            : Level_(level)
            , Log_(log)
        {
            Y_ENSURE(Log_);
        }

        TNgxStreamLogger Stream() {
            return TNgxStreamLogger(Level_, Log_);
        }

        template <typename... Args>
        void Log(char const* const format, const Args&... args) {
            ngx_log_error(Level_, Log_, 0, format, args...);
        }

        // make TNgxLogger with another level
        TNgxLogger operator()(const ngx_uint_t level) {
            return TNgxLogger(level, Log_);
        }

    private:
        const ngx_uint_t Level_;
        ngx_log_t* const Log_;
    };
}
