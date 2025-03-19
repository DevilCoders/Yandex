#pragma once

#include "public.h"

#include <util/datetime/base.h>
#include <util/generic/string.h>

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

namespace NCloud::NBlockStore::NRdma::NVerbs {

////////////////////////////////////////////////////////////////////////////////

#define RDMA_DECLARE_PTR(name, type, dereg)                                    \
    using T##name##Ptr = std::unique_ptr<type, decltype(&dereg)>;              \
    inline T##name##Ptr WrapPtr(type* ptr) { return { ptr, dereg }; }          \
// RDMA_DECLARE_PTR

RDMA_DECLARE_PTR(Context, ibv_context, ibv_close_device);
RDMA_DECLARE_PTR(DeviceList, ibv_device*, ibv_free_device_list);
RDMA_DECLARE_PTR(ProtectionDomain, ibv_pd, ibv_dealloc_pd);
RDMA_DECLARE_PTR(MemoryRegion, ibv_mr, ibv_dereg_mr);
RDMA_DECLARE_PTR(CompletionChannel, ibv_comp_channel, ibv_destroy_comp_channel);
RDMA_DECLARE_PTR(CompletionQueue, ibv_cq, ibv_destroy_cq);
RDMA_DECLARE_PTR(AddressInfo, rdma_addrinfo, rdma_freeaddrinfo);
RDMA_DECLARE_PTR(EventChannel, rdma_event_channel, rdma_destroy_event_channel);
RDMA_DECLARE_PTR(ConnectionEvent, rdma_cm_event, rdma_ack_cm_event);
RDMA_DECLARE_PTR(Connection, rdma_cm_id, rdma_destroy_id);

#undef RDMA_DECLARE_PTR

////////////////////////////////////////////////////////////////////////////////

struct TNullPtr
{
    template <typename T, typename F>
    operator std::unique_ptr<T, F> () const
    {
        return { nullptr, nullptr };
    }
};

constexpr TNullPtr NullPtr;

////////////////////////////////////////////////////////////////////////////////

TContextPtr OpenDevice(ibv_device* device);
TDeviceListPtr GetDeviceList();
void QueryDevice(ibv_context* context, ibv_device_attr_ex* device_attr);
ui64 GetDeviceTimestamp(ibv_context* context);

TProtectionDomainPtr CreateProtectionDomain(ibv_context* context);
TMemoryRegionPtr RegisterMemoryRegion(
    ibv_pd* pd,
    void* addr,
    size_t length,
    int flags);

TCompletionChannelPtr CreateCompletionChannel(ibv_context* context);
TCompletionQueuePtr CreateCompletionQueue(
    ibv_context* context,
    ibv_cq_init_attr_ex* cq_attrs);

void RequestCompletionEvent(ibv_cq* cq, int solicited_only);
void* GetCompletionEvent(ibv_cq* cq);
void AckCompletionEvents(ibv_cq* cq, unsigned int nevents);

struct TCompletion
{
    ui64 wr_id;
    ibv_wc_status status;
    ibv_wc_opcode opcode;
    ui64 ts;
};

struct ICompletionHandler
{
    virtual ~ICompletionHandler() = default;
    virtual void HandleCompletionEvent(const TCompletion& wc) = 0;
};

bool PollCompletionQueue(ibv_cq_ex* cq, ICompletionHandler* handler);

void PostSend(ibv_qp* qp, ibv_send_wr* wr);
void PostRecv(ibv_qp* qp, ibv_recv_wr* wr);

////////////////////////////////////////////////////////////////////////////////

TAddressInfoPtr GetAddressInfo(
    const TString& host,
    ui32 port,
    rdma_addrinfo* hints);

TEventChannelPtr CreateEventChannel();
TConnectionEventPtr GetConnectionEvent(rdma_event_channel* channel);

TConnectionPtr CreateConnection(
    rdma_event_channel* channel,
    void* context,
    rdma_port_space ps);

void BindAddress(rdma_cm_id* id, sockaddr* addr);
void ResolveAddress(
    rdma_cm_id* id,
    sockaddr* src_addr,
    sockaddr* dst_addr,
    TDuration timeout);
void ResolveRoute(rdma_cm_id* id, TDuration timeout);

void Listen(rdma_cm_id* id, int backlog);
void Connect(rdma_cm_id* id, rdma_conn_param* conn_param);
void Accept(rdma_cm_id* id, rdma_conn_param* conn_param);
void Reject(rdma_cm_id* id, const void* private_data, ui8 private_data_len);

void CreateQP(rdma_cm_id* id, ibv_qp_init_attr* qp_attrs);
void DestroyQP(rdma_cm_id* id);

////////////////////////////////////////////////////////////////////////////////

const char* GetStatusString(ibv_wc_status status);
const char* GetOpcodeName(ibv_wc_opcode opcode);
const char* GetEventName(rdma_cm_event_type event);

TString PrintAddress(const sockaddr* addr);
TString PrintConnectionParams(const rdma_conn_param* conn_param);

}   // namespace NCloud::NBlockStore::NRdma::NVerbs
