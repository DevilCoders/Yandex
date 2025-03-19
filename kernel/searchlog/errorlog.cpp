#include <library/cpp/logger/stream.h>
#include <library/cpp/logger/thread.h>

#include <util/generic/cast.h>
#include <util/system/backtrace.h>
#include <util/stream/str.h>
#include <util/stream/printf.h>

#include "errorlog.h"

TErrorLogElement::TErrorLogElement(TErrorLog* parent, ELogPriority priority, const TSourceLocation& location)
    : TLogElement(parent->Log(), priority)
    , ErrorLog(parent)
    , Location(location)
{
}

inline void WriteHeader(ELogPriority priority, IOutputStream& stream) {
    stream << FormatLocal(Now()) << "\t";
    switch(priority) {
        case TLOG_EMERG:
            stream << "[EMERGENCY]\t";
            break;
        case TLOG_CRIT:
            stream << "[CRITICAL]\t";
            break;
        case TLOG_ERR:
            stream << "[ERROR]\t";
            break;
        case TLOG_WARNING:
            stream << "[WARNING]\t";
            break;
        case TLOG_INFO:
            stream << "[INFO]\t";
            break;
        case TLOG_DEBUG:
            stream << "[DEBUG]\t";
            break;
        default:
            stream << "[UNKNOWN]\t";
            break;
    }
}

inline void WriteFooter(ELogPriority priority, const TSourceLocation& location, const TErrorLog* errorLog, IOutputStream& stream) {
    stream << " (" << location.File << ":" << location.Line << ")";
    if (priority <= TLOG_ERR) {
        errorLog->PrintRequestInfo(stream);
    }
    stream << '\n';
}

TErrorLogElement::~TErrorLogElement() {
    const size_t filled = Filled();
    if (filled <= HeaderLength) {
        Reset(); // keep log clean from unused/empty CreateLogEntry() objects
    } else if (!UncaughtException()) {
        try {
            Y_ASSERT(Location.Line != -1);
            if (Location.Line != -1) {
                if (Data()[Filled() - 1] == '\n') {
                    SetPos(Filled() - 1);
                }
                WriteFooter(Priority_, Location,
                    static_cast<const TErrorLog*>(Parent_), static_cast<IOutputStream&>(*this));
            } else {
                if (Data()[filled - 1] != '\n') {
                    static_cast<IOutputStream&>(*this) << '\n';
                }
            }
            if (Parent_->IsOpen() && static_cast<const TErrorLog*>(Parent_)->ReplicateMessagesToErrorStream) {
                Cerr << TStringBuf(Data(), Filled());
            }
            ErrorLog->SetLastEntry(*this);
        } catch (...) {
        }
    }
}
TErrorLog::TErrorLog()
    : ReplicateMessagesToErrorStream(false)
{
    // Log to stderr by default.
    ResetBackend(MakeHolder<TOwningThreadedLogBackend>(new TStreamLogBackend(&Cerr)));
    PreallocatedMessage.reserve(PREALLOCATED_MESSAGE_LENGTH); // Reserve memory for some message
        // and search request
    // by default keep last errors info
    for (size_t i = 0; i < LogPrioritiesCount_; ++i) {
        KeepLastEntry_[i] = ((size_t)i <= TLOG_ERR);
    }
}

THolder<TErrorLogElement> TErrorLog::CreateLogEntry(ELogPriority priority, TSourceLocation location) {
    THolder<TErrorLogElement> element(new TErrorLogElement(this, priority, location));
    WriteHeader(priority, *element);
    element->FixHeaderLength();
    {
        TReadGuard guard(MessageCallbackMutex);
        if (const TMessageCallback* callback = MessageCallbacks.FindPtr(priority)) {
            (*callback)();
        }
    }
    return element;
}

void TErrorLog::WriteFormattedLogEntry(ELogPriority priority, TSourceLocation location, bool printBackTrace, const char* format, ...) const {
    TString errorMessage;
    TStringOutput errorStream(errorMessage);
    WriteHeader(priority, errorStream);

    va_list args;
    va_start(args, format);
    Printf(errorStream, format, args);
    va_end(args);

    if (errorMessage.back() == '\n') {
        errorMessage.pop_back();
    }

    WriteFooter(priority, location, this, errorStream);

#ifndef WITH_VALGRIND
    if (printBackTrace) {
        FormatBackTrace(&errorStream);
    }
#else
    Y_UNUSED(printBackTrace);
#endif

    Write(priority, errorMessage.data(), errorMessage.size());
    if (IsOpen() && ReplicateMessagesToErrorStream) {
        Cerr << static_cast<TString&>(errorMessage);
    }
}

void TErrorLog::ReportMemoryAllocationProblems() {
    TLogRecord logRec(TLOG_EMERG, nullptr, 0);
    TStringOutput messageStream(PreallocatedMessage);

    WriteHeader(TLOG_EMERG, messageStream);
    messageStream << "Critical error related to memory allocation ";
    Y_ASSERT(PreallocatedMessage.size() + 1 < PREALLOCATED_MESSAGE_LENGTH);
    PrintRequestInfo(messageStream, PREALLOCATED_MESSAGE_LENGTH - PreallocatedMessage.size() - 1);
    messageStream << '\n';
    if (IsOpen()) {
        TAutoPtr<TLogBackend> backend = ReleaseBackend();
        TThreadedLogBackend* threadedBackend = dynamic_cast<TThreadedLogBackend*>(backend.Get());
        Y_ASSERT(threadedBackend != nullptr);
        logRec.Data = PreallocatedMessage.c_str();
        logRec.Len = PreallocatedMessage.length();
        if (threadedBackend != nullptr) {
            threadedBackend->WriteEmergencyData(logRec);
            if (ReplicateMessagesToErrorStream) {
                Cerr << PreallocatedMessage;
            }
        } else {
            Cerr << PreallocatedMessage;
        }
    } else {
        Cerr << PreallocatedMessage;
    }
}

void TErrorLog::RegisterRequestInfo(TRequestInfo&& requestInfo) {
    TGuard<TMutex> guard(RequestInfosMutex);
    TThread::TId threadId = TThread::CurrentThreadId();
    if (threadId != TThread::ImpossibleThreadId()) {
        RequestInfos[threadId] = std::move(requestInfo);
    }
}

void TErrorLog::PrintRequestInfo(IOutputStream& out, size_t maxSize) const {
    TGuard<TMutex> guard(RequestInfosMutex);
    const auto* reqInfo = RequestInfos.FindPtr(TThread::CurrentThreadId());
    if (reqInfo != nullptr) {
        if (maxSize == 0 || (2 + reqInfo->CollectionName.size() + 2 + reqInfo->QueryString.size() + 1) <= maxSize) {
            out << " (" << reqInfo->CollectionName << ": " << reqInfo->QueryString << ')';
        } else {
            if (maxSize > 2 + reqInfo->CollectionName.size() + 2) {
                out << " (" << reqInfo->CollectionName << ": ";
                maxSize -= 2 + reqInfo->CollectionName.size() + 2;
                Y_ASSERT(maxSize > 0 && maxSize <= reqInfo->QueryString.size());
                out.Write(reqInfo->QueryString.data(), maxSize);
            }
        }
    }
}

void TErrorLog::DeleteRequestInfo() {
    TGuard<TMutex> guard(RequestInfosMutex);
    RequestInfos.erase(TThread::CurrentThreadId());
}

void TErrorLog::KeepLastEntry(ELogPriority priority, bool enable) {
    KeepLastEntry_[priority] = enable;
    if (!enable) {
        TString s;
        {
            TGuard<TSpinLock> g(SL_);
            LastEntry_[priority].swap(s);
        }
    }
}

void TErrorLog::SetLastEntry(const TLogElement& logElem) {
    if (KeepLastEntry_[logElem.Priority()]) {
        TString s(logElem.Data(), logElem.Filled());
        {
            TGuard<TSpinLock> g(SL_);
            LastEntry_[logElem.Priority()].swap(s);
        }
    }
}

TString TErrorLog::LastEntry(ELogPriority priority) {
    TString s;
    {
        TGuard<TSpinLock> g(SL_);
        s = LastEntry_[priority];
    }
    return s;
}


void TErrorLog::SetCallbackOnPriorityMessage(ELogPriority priority, TMessageCallback callback) {
    TWriteGuard guard(MessageCallbackMutex);
    MessageCallbacks.insert(std::make_pair(priority, callback));
}

