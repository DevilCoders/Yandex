#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>

#include <util/system/rwlock.h>

namespace google {
    namespace protobuf {
        class Message;
    }
}

namespace NQueryData {

    template <class TObj>
    class TGuarded : TNonCopyable {
    public:
        typedef TGuarded<TObj> TSelf;
        typedef TObj TObject;

        class TRead : TNonCopyable {
            TReadGuard Guard;
        public:
            const TObject& Object;

            operator const TObject&() const {
                return Object;
            }

            TRead(const TSelf& coll)
                : Guard(coll.Mutex)
                , Object(coll.Object)
            {}
        };

        class TWrite : TNonCopyable {
            TWriteGuard Guard;
        public:
            TObject& Object;

            operator TObject&() {
                return Object;
            }

            TWrite(TSelf& coll)
                : Guard(coll.Mutex)
                , Object(coll.Object)
            {}
        };

        TObject& Unsafe() {
            return Object;
        }

        const TObject& Unsafe() const {
            return Object;
        }

    private:
        TObject Object;
        TRWMutex Mutex;
    };

    ui64 CurrentMemoryUsage();

    time_t FastNow();

    TString GetPath(const TString& defdir, const TString& file);

    TString TimestampToString(time_t);

    TString HumanReadableProtobuf(const google::protobuf::Message&, bool compact = true);

}
