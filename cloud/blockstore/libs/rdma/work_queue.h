#pragma once

#include "public.h"

#include <rdma/rdma_verbs.h>

#include <util/generic/utility.h>

namespace NCloud::NBlockStore::NRdma {

////////////////////////////////////////////////////////////////////////////////

constexpr size_t RDMA_MAX_SEND_SGE = 1;
constexpr size_t RDMA_MAX_RECV_SGE = 1;

////////////////////////////////////////////////////////////////////////////////

struct TSendWr
{
    ibv_send_wr wr;
    ibv_sge sg_list[RDMA_MAX_SEND_SGE];

    void* context;

    TSendWr()
    {
        Zero(*this);
    }

    template <typename T>
    T* Message()
    {
        return reinterpret_cast<T*>(wr.sg_list[0].addr);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TRecvWr
{
    ibv_recv_wr wr;
    ibv_sge sg_list[RDMA_MAX_RECV_SGE];

    void* context;

    TRecvWr()
    {
        Zero(*this);
    }

    template <typename T>
    T* Message()
    {
        return reinterpret_cast<T*>(wr.sg_list[0].addr);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class TWorkQueue
{
private:
    T* Head = nullptr;

public:
    void Push(T* wr)
    {
        wr->wr.next = &Head->wr;    // wr must have 0 offset for this to work
        Head = wr;                  // properly
    }

    T* Pop()
    {
        auto* wr = Head;
        if (wr) {
            Head = reinterpret_cast<T*>(wr->wr.next);
            wr->wr.next = nullptr;
        }
        return wr;
    }

    void Clear()
    {
        Head = nullptr;
    }
};

}   // namespace NCloud::NBlockStore::NRdma
