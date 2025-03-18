#pragma once

extern "C" {
#include <ngx_core.h>
#include <ngx_cycle.h>
}

#include <nginx/modules/strm_packager/src/base/pool_util.h>

#include <util/generic/yexception.h>
#include <util/string/builder.h>

namespace NStrm::NPackager {
    class TShmMutex {
    public:
        void Acquire() {
            ngx_shmtx_lock(Mutex);
        }
        void Release() {
            ngx_shmtx_unlock(Mutex);
        }

    private:
        explicit TShmMutex(ngx_shmtx_t* mutex)
            : Mutex(mutex)
        {
            Y_ENSURE(Mutex);
        }

        ngx_shmtx_t* const Mutex;

        template <typename TData>
        friend class TShmZone;
    };

    // TData must have
    //   > default constructor
    //   > operator= (const TData&)
    //   > void TData::Init(TShmZone<TData>& zone) - it must allocate and init TData::TState, and set it by SetShmState
    //   > void TData::InitWithExistingShmState(TShmZone<TData>& zone) - init with TData::TState already existed in the zone
    //   here `void*` result of Init will be passed to `void* state` in InitFromShmState in some cases (see InitZone)
    //     it must be pointer, allocated in shared memory
    template <typename TData>
    class TShmZone {
    public:
        static TShmZone CreateNew(ngx_str_t name, const size_t size, ngx_conf_t* cf);
        static TShmZone GetExisting(ngx_str_t name, ngx_conf_t* cf);

        TShmZone(const TShmZone&) = default;

        TData& Data() const;

        size_t Size() const;

        // Alloc, Free, SetShmState, GetShmState must be executed under this mutex
        TShmMutex GetMutex();
        ui8* Alloc(size_t size);
        ui8* AllocNoexcept(size_t size); // return nullptr if failed
        void Free(ui8*);
        void SetShmState(typename TData::TState*);
        typename TData::TState& GetShmState();

    private:
        explicit TShmZone(ngx_shm_zone_t* zone);

        ngx_slab_pool_t* Shpool();
        typename TData::TState*& ShmState();

        static ngx_int_t InitZone(ngx_shm_zone_t* zone, void* data);

        static int TAG_DATA;

        ngx_shm_zone_t* const Zone;
    };

    // ======= implementation

    template <typename TData>
    int TShmZone<TData>::TAG_DATA = 0;

    // static
    template <typename TData>
    TShmZone<TData> TShmZone<TData>::CreateNew(ngx_str_t name, const size_t size, ngx_conf_t* cf) {
        TData* data = TNgxPoolUtil<TData>::NewInPool(cf->pool);

        ngx_shm_zone_t* zone = ::ngx_shared_memory_add(
            cf,
            &name,
            size,
            &TAG_DATA);

        Y_ENSURE(zone);
        Y_ENSURE(!zone->data);

        zone->data = data;
        zone->init = &InitZone;

        return TShmZone(zone);
    }

    // static
    template <typename TData>
    TShmZone<TData> TShmZone<TData>::GetExisting(ngx_str_t name, ngx_conf_t* cf) {
        ngx_shm_zone_t* zone = ::ngx_shared_memory_add(
            cf,
            &name,
            0, // = size
            &TAG_DATA);

        Y_ENSURE(zone);
        Y_ENSURE(zone->data);

        return TShmZone(zone);
    }

    // static
    template <typename TData>
    ngx_int_t TShmZone<TData>::InitZone(ngx_shm_zone_t* zonePtr, void* dataPtr) try {
        Y_ENSURE(zonePtr);
        Y_ENSURE(zonePtr->shm.addr);

        TShmZone<TData> zone(zonePtr);

        {
            TShmMutex shmMutex = zone.GetMutex();
            const TGuard<TShmMutex> shmGuard(shmMutex);

            if (dataPtr || zone.Zone->shm.exists) {
                Y_ENSURE(zone.ShmState());
                zone.Data().InitWithExistingShmState(zone);
            } else {
                Y_ENSURE(!zone.ShmState());
                zone.Data().Init(zone);
            }
            Y_ENSURE(zone.ShmState());

            zone.Shpool()->log_nomem = 0;
        }

        return NGX_OK;
    } catch (const std::exception& e) {
        Cout << (TStringBuilder() << "exception '" << e.what() << "'  in TShmZone<TData>::InitZone \n");
        Cout.Flush();
        return NGX_ERROR;
    } catch (...) {
        Cout << (TStringBuilder() << "exception '...'  in TShmZone<TData>::InitZone \n");
        Cout.Flush();
        return NGX_ERROR;
    }

    template <typename TData>
    ngx_slab_pool_t* TShmZone<TData>::Shpool() {
        return (ngx_slab_pool_t*)Zone->shm.addr;
    }

    template <typename TData>
    TShmMutex TShmZone<TData>::GetMutex() {
        return TShmMutex(&Shpool()->mutex);
    }

    template <typename TData>
    ui8* TShmZone<TData>::AllocNoexcept(size_t size) {
        return (ui8*)::ngx_slab_alloc_locked(Shpool(), size);
    }

    template <typename TData>
    ui8* TShmZone<TData>::Alloc(size_t size) {
        Y_ENSURE(size > 0);
        ui8* ret = AllocNoexcept(size);
        if (!ret) {
            throw std::bad_alloc();
        }
        return ret;
    }

    template <typename TData>
    void TShmZone<TData>::Free(ui8* data) {
        Y_ENSURE(data);
        ::ngx_slab_free_locked(Shpool(), data);
    }

    template <typename TData>
    TShmZone<TData>::TShmZone(ngx_shm_zone_t* zone)
        : Zone(zone)
    {
        Y_ENSURE(Zone);
    }

    template <typename TData>
    TData& TShmZone<TData>::Data() const {
        Y_ENSURE(Zone->data);
        return *(TData*)Zone->data;
    }

    template <typename TData>
    size_t TShmZone<TData>::Size() const {
        return Zone->shm.size;
    }

    template <typename TData>
    typename TData::TState*& TShmZone<TData>::ShmState() {
        return (typename TData::TState*&)Shpool()->data;
    }

    template <typename TData>
    void TShmZone<TData>::SetShmState(typename TData::TState* newState) {
        Y_ENSURE(newState);
        typename TData::TState*& state = ShmState();
        Y_ENSURE(!state);
        state = newState;
    }

    template <typename TData>
    typename TData::TState& TShmZone<TData>::GetShmState() {
        typename TData::TState*& state = ShmState();
        Y_ENSURE(state);
        return *state;
    }

}
