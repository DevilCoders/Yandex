#pragma once

#if !defined(FILESTORE_REQUEST_H)
#error "should not be included directly - include request.h instead"
#endif

#include <util/generic/typetraits.h>

namespace NCloud::NFileStore {

namespace NImpl {

////////////////////////////////////////////////////////////////////////////////

Y_HAS_MEMBER(GetFileSystemId);

template <typename T, typename = void>
struct TGetFileSystemIdSelector;

template <typename T>
struct TGetFileSystemIdSelector<T, std::enable_if_t<THasGetFileSystemId<T>::value>>
{
    static TString Get(const T& request)
    {
        return request.GetFileSystemId();
    }
};

template <typename T>
struct TGetFileSystemIdSelector<T, std::enable_if_t<!THasGetFileSystemId<T>::value>>
{
    static TString Get(const T&)
    {
        return {};
    }
};

template <typename T>
TString DoGetFileSystemId(const T& request)
{
    return TGetFileSystemIdSelector<T>::Get(request);
}

////////////////////////////////////////////////////////////////////////////////

Y_HAS_MEMBER(GetNodeId);

template <typename T, typename = void>
struct TGetNodeIdSelector;

template <typename T>
struct TGetNodeIdSelector<T, std::enable_if_t<THasGetNodeId<T>::value>>
{
    static ui64 Get(const T& request)
    {
        return request.GetNodeId();
    }
};

template <typename T>
struct TGetNodeIdSelector<T, std::enable_if_t<!THasGetNodeId<T>::value>>
{
    static ui64 Get(const T&)
    {
        return 0;
    }
};

template <typename T>
ui64 DoGetNodeId(const T& request)
{
    return TGetNodeIdSelector<T>::Get(request);
}

}    // namespace NImpl

////////////////////////////////////////////////////////////////////////////////

template <typename T>
TString GetClientId(const T& request)
{
    return request.GetHeaders().GetClientId();
}

template <typename T>
TString GetSessionId(const T& request)
{
    return request.GetHeaders().GetSessionId();
}

template <typename T>
ui64 GetRequestId(const T& request)
{
    return request.GetHeaders().GetRequestId();
}

template <typename T>
TString GetFileSystemId(const T& request)
{
    return NImpl::DoGetFileSystemId(request);
}

template <typename T>
ui64 GetNodeId(const T& request)
{
    return NImpl::DoGetNodeId(request);
}

template <typename T>
TString GetRequestName(const T& request)
{
    Y_UNUSED(request);
    return T::descriptor()->name();
}

template <typename T>
TRequestInfo GetRequestInfo(const T& request)
{
    return {
        GetRequestId(request),
        GetRequestName(request),
        GetFileSystemId(request),
        GetSessionId(request),
        GetClientId(request),
    };
}

}   // namespace NCloud::NFileStore
