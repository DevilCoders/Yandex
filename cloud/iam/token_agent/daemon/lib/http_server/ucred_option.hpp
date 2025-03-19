#pragma once

#include <asio.hpp>

namespace NHttpServer {
    class TUcredOption {
    public:
        pid_t Pid() const {
            return value_.pid;
        }

        uid_t Uid() const {
            return value_.uid;
        }

        gid_t Gid() const {
            return value_.gid;
        }

        template<typename Protocol>
        int level(const Protocol&) const {
            return SOL_SOCKET;
        }

        template<typename Protocol>
        int name(const Protocol&) const {
            return SO_PEERCRED;
        }

        template<typename Protocol>
        ucred* data(const Protocol&) {
            return &value_;
        }

        template<typename Protocol>
        const ucred* data(const Protocol&) const {
            return &value_;
        }

        template<typename Protocol>
        std::size_t size(const Protocol&) const {
            return sizeof(value_);
        }

        template<typename Protocol>
        void resize(const Protocol&, std::size_t s) {
            if (s != sizeof(value_)) {
                std::length_error ex("ucred socket option resize");
                asio::detail::throw_exception(ex);
            }
        }

    private:
        ucred value_;
    };
}
