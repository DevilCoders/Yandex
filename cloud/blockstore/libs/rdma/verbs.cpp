#include "verbs.h"

#include <cloud/storage/core/libs/common/error.h>

#include <util/network/address.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/system/error.h>

namespace NCloud::NBlockStore::NRdma::NVerbs {

////////////////////////////////////////////////////////////////////////////////

#define RDMA_THROW_ERROR(method)                                               \
    throw TServiceError(MAKE_SYSTEM_ERROR(LastSystemError()))                  \
        << method << " failed with error " << LastSystemError()                \
        << ": " << LastSystemErrorText()                                       \
// RDMA_THROW_ERROR

////////////////////////////////////////////////////////////////////////////////

TDeviceListPtr GetDeviceList()
{
    auto** list = ibv_get_device_list(nullptr);
    return WrapPtr(list);
}

TContextPtr OpenDevice(ibv_device* device)
{
    auto* context = ibv_open_device(device);
    if (!context) {
        RDMA_THROW_ERROR("ibv_open_device");
    }

    return WrapPtr(context);
}

void QueryDevice(ibv_context* context, ibv_device_attr_ex* device_attr)
{
    int res = rdma_seterrno(ibv_query_device_ex(context, nullptr, device_attr));
    if (res < 0) {
        RDMA_THROW_ERROR("ibv_query_device_ex");
    }
}

ui64 GetDeviceTimestamp(ibv_context* context)
{
    ibv_values_ex values = {
        .comp_mask = IBV_VALUES_MASK_RAW_CLOCK,
    };

    int res = rdma_seterrno(ibv_query_rt_values_ex(context, &values));
    if (res < 0) {
        RDMA_THROW_ERROR("ibv_query_rt_values_ex");
    }

    return (ui64)values.raw_clock.tv_sec * 1000000000
         + (ui64)values.raw_clock.tv_nsec;
}

TProtectionDomainPtr CreateProtectionDomain(ibv_context* context)
{
    auto* pd = ibv_alloc_pd(context);
    if (!pd) {
        RDMA_THROW_ERROR("ibv_alloc_pd");
    }

    return WrapPtr(pd);
}

// ibdrv replaces ibv_reg_mr with macro that uses ibv_reg_mr_iova2 if
// IBV_ACCESS_OPTIONAL_RANGE flag is present or (for some reason) if compiler
// can't deduce whether `flags` is constant or not. This will crash on older
// versions where ibv_reg_mr_iova2 is not present, but makes no difference on
// newer ones, since it calls _iova2 variant anyway

#ifdef ibv_reg_mr
#undef ibv_reg_mr
#endif

TMemoryRegionPtr RegisterMemoryRegion(
    ibv_pd* pd,
    void* addr,
    size_t length,
    int flags)
{
    auto* mr = ibv_reg_mr(pd, addr, length, flags);
    if (!mr) {
        RDMA_THROW_ERROR("ibv_reg_mr");
    }

    return WrapPtr(mr);
}

TCompletionChannelPtr CreateCompletionChannel(ibv_context* context)
{
    auto* channel = ibv_create_comp_channel(context);
    if (!channel) {
        RDMA_THROW_ERROR("ibv_create_comp_channel");
    }

    return WrapPtr(channel);
}

TCompletionQueuePtr CreateCompletionQueue(
    ibv_context* context,
    ibv_cq_init_attr_ex* cq_attrs)
{
    auto* cq_ex = ibv_create_cq_ex(context, cq_attrs);
    if (!cq_ex) {
        RDMA_THROW_ERROR("ibv_create_cq_ex");
    }

    return WrapPtr(ibv_cq_ex_to_cq(cq_ex));
}

void RequestCompletionEvent(ibv_cq* cq, int solicited_only)
{
    int res = rdma_seterrno(ibv_req_notify_cq(cq, solicited_only));
    if (res < 0) {
        RDMA_THROW_ERROR("ibv_req_notify_cq");
    }
}

void* GetCompletionEvent(ibv_cq* cq)
{
    ibv_cq* ev_cq;
    void* ev_ctx;

    int res = ibv_get_cq_event(cq->channel, &ev_cq, &ev_ctx);
    if (res < 0) {
        RDMA_THROW_ERROR("ibv_get_cq_event");
    }

    Y_VERIFY(ev_cq == cq);
    return ev_ctx;
}

void AckCompletionEvents(ibv_cq* cq, unsigned int nevents)
{
    ibv_ack_cq_events(cq, nevents);
}

bool PollCompletionQueue(ibv_cq_ex* cq, ICompletionHandler* handler)
{
    ibv_poll_cq_attr attrs = {};
    if (ibv_start_poll(cq, &attrs) == ENOENT) {
        return false;
    }

    TCompletion wc = {};
    do {
        wc.wr_id = cq->wr_id;
        wc.status = cq->status;
        wc.opcode = ibv_wc_read_opcode(cq);
        wc.ts = ibv_wc_read_completion_ts(cq);

        handler->HandleCompletionEvent(wc);
    } while (ibv_next_poll(cq) != ENOENT);

    ibv_end_poll(cq);
    return true;
}

void PostSend(ibv_qp* qp, ibv_send_wr* wr)
{
    ibv_send_wr* bad = nullptr;

    int res = rdma_seterrno(ibv_post_send(qp, wr, &bad));
    if (res < 0) {
        RDMA_THROW_ERROR("ibv_post_send");
    }
}

void PostRecv(ibv_qp* qp, ibv_recv_wr* wr)
{
    ibv_recv_wr* bad = nullptr;

    int res = rdma_seterrno(ibv_post_recv(qp, wr, &bad));
    if (res < 0) {
        RDMA_THROW_ERROR("ibv_post_recv");
    }
}

////////////////////////////////////////////////////////////////////////////////

TAddressInfoPtr GetAddressInfo(
    const TString& host,
    ui32 port,
    rdma_addrinfo* hints)
{
    rdma_addrinfo* addr = nullptr;

    int res = rdma_getaddrinfo(
        host.c_str(),
        ToString(port).c_str(),
        hints,
        &addr);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_getaddrinfo");
    }

    return WrapPtr(addr);
}

TEventChannelPtr CreateEventChannel()
{
    auto* channel = rdma_create_event_channel();
    if (!channel) {
        RDMA_THROW_ERROR("rdma_create_event_channel");
    }

    return WrapPtr(channel);
}

TConnectionEventPtr GetConnectionEvent(rdma_event_channel* channel)
{
    rdma_cm_event* event = nullptr;

    int res = rdma_get_cm_event(channel, &event);
    if (res < 0) {
        if (errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK) {
            RDMA_THROW_ERROR("rdma_get_cm_event");
        }
        return NullPtr;
    }

    return WrapPtr(event);
}

TConnectionPtr CreateConnection(
    rdma_event_channel* channel,
    void* context,
    rdma_port_space ps)
{
    rdma_cm_id* id = nullptr;

    int res = rdma_create_id(channel, &id, context, ps);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_create_id");
    }

    return WrapPtr(id);
}

void BindAddress(rdma_cm_id* id, sockaddr* addr)
{
    int res = rdma_bind_addr(id, addr);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_bind_addr");
    }
}

void ResolveAddress(
    rdma_cm_id* id,
    sockaddr* src_addr,
    sockaddr* dst_addr,
    TDuration timeout)
{
    int res = rdma_resolve_addr(id, src_addr, dst_addr, timeout.MilliSeconds());
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_resolve_addr");
    }
}

void ResolveRoute(rdma_cm_id* id, TDuration timeout)
{
    int res = rdma_resolve_route(id, timeout.MilliSeconds());
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_resolve_route");
    }
}

void Listen(rdma_cm_id* id, int backlog)
{
    int res = rdma_listen(id, backlog);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_listen");
    }
}

void Connect(rdma_cm_id* id, rdma_conn_param* conn_param)
{
    int res = rdma_connect(id, conn_param);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_connect");
    }
}

void Accept(rdma_cm_id* id, rdma_conn_param* conn_param)
{
    int res = rdma_accept(id, conn_param);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_accept");
    }
}

void Reject(rdma_cm_id* id, const void* private_data, ui8 private_data_len)
{
    int res = rdma_reject(id, private_data, private_data_len);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_reject");
    }
}

void CreateQP(rdma_cm_id* id, ibv_qp_init_attr* qp_attrs)
{
    int res = rdma_create_qp(id, id->pd, qp_attrs);
    if (res < 0) {
        RDMA_THROW_ERROR("rdma_create_qp");
    }
}

void DestroyQP(rdma_cm_id* id)
{
    rdma_destroy_qp(id);
}

////////////////////////////////////////////////////////////////////////////////

const char* GetOpcodeName(ibv_wc_opcode opcode)
{
    static const char* names[] = {
        "IBV_WC_SEND",
        "IBV_WC_RDMA_WRITE",
        "IBV_WC_RDMA_READ",
        "IBV_WC_COMP_SWAP",
        "IBV_WC_FETCH_ADD",
        "IBV_WC_BIND_MW",
        "IBV_WC_LOCAL_INV",
        "IBV_WC_TSO",
    };

    static const char* names2[] = {
        "IBV_WC_RECV",
        "IBV_WC_RECV_RDMA_WITH_IMM",
        "IBV_WC_TM_ADD",
        "IBV_WC_TM_DEL",
        "IBV_WC_TM_SYNC",
        "IBV_WC_TM_RECV",
        "IBV_WC_TM_NO_TAG",
        "IBV_WC_DRIVER1",
        "IBV_WC_DRIVER2",
        "IBV_WC_DRIVER3",
    };

    if ((size_t)opcode < Y_ARRAY_SIZE(names)) {
        return names[(size_t)opcode];
    }

    if ((size_t)opcode - IBV_WC_RECV < Y_ARRAY_SIZE(names2)) {
        return names2[(size_t)opcode - IBV_WC_RECV];
    }

    return "IBV_WC_UNKNOWN";
}

const char* GetStatusString(ibv_wc_status status)
{
    return ibv_wc_status_str(status);
}

const char* GetEventName(rdma_cm_event_type event)
{
    static const char* names[] = {
        "RDMA_CM_EVENT_ADDR_RESOLVED",
        "RDMA_CM_EVENT_ADDR_ERROR",
        "RDMA_CM_EVENT_ROUTE_RESOLVED",
        "RDMA_CM_EVENT_ROUTE_ERROR",
        "RDMA_CM_EVENT_CONNECT_REQUEST",
        "RDMA_CM_EVENT_CONNECT_RESPONSE",
        "RDMA_CM_EVENT_CONNECT_ERROR",
        "RDMA_CM_EVENT_UNREACHABLE",
        "RDMA_CM_EVENT_REJECTED",
        "RDMA_CM_EVENT_ESTABLISHED",
        "RDMA_CM_EVENT_DISCONNECTED",
        "RDMA_CM_EVENT_DEVICE_REMOVAL",
        "RDMA_CM_EVENT_MULTICAST_JOIN",
        "RDMA_CM_EVENT_MULTICAST_ERROR",
        "RDMA_CM_EVENT_ADDR_CHANGE",
        "RDMA_CM_EVENT_TIMEWAIT_EXIT"
    };

    if ((size_t)event < Y_ARRAY_SIZE(names)) {
        return names[(size_t)event];
    }

    return "RDMA_CM_EVENT_UNKNOWN";
}

TString PrintAddress(const sockaddr* addr)
{
    return NAddr::PrintHostAndPort(NAddr::TOpaqueAddr(addr));
}

TString PrintConnectionParams(const rdma_conn_param* conn_param)
{
    return TStringBuilder()
        << "[private_data=" << Hex((uintptr_t)conn_param->private_data)
        << ", private_data_len=" << (uint32_t)conn_param->private_data_len
        << ", responder_resources=" << (uint32_t)conn_param->responder_resources
        << ", initiator_depth=" << (uint32_t)conn_param->initiator_depth
        << ", flow_control=" << (uint32_t)conn_param->flow_control
        << ", retry_count=" << (uint32_t)conn_param->retry_count
        << ", rnr_retry_count=" << (uint32_t)conn_param->rnr_retry_count
        << ", srq=" << (uint32_t)conn_param->srq
        << ", qp_num=" << (uint32_t)conn_param->qp_num
        << "]";
}

}   // namespace NCloud::NBlockStore::NRdma::NVerbs
