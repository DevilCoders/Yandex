#include <nginx/modules/strm_packager/src/base/shm_cache.h>

namespace NStrm::NPackager {
    void TShmCache::SetSettings(const TSettings& settings) {
        Y_ENSURE(!Zone);
        ToInitSettings = settings;
    }

    void TShmCache::Init(TZone& zone) {
        Y_ENSURE(!Zone);
        Zone.ConstructInPlace(zone);

        TState* state = (TState*)Zone->Alloc(sizeof(TState));
        Zone->SetShmState(state);

        ngx_rbtree_init(
            &state->RBTree,
            &state->RBTreeSentinel,
            TShmCache::RBTreeInsertValue);

        ngx_queue_init(&state->QReady);
        ngx_queue_init(&state->QInProgress);
        ngx_queue_init(&state->QAccessOrder);

        Y_ENSURE(ToInitSettings.Defined());
        state->Settings = *ToInitSettings;
    }

    void TShmCache::InitWithExistingShmState(TZone& zone) {
        Y_ENSURE(!Zone);
        Zone.ConstructInPlace(zone);

        Y_ENSURE(ToInitSettings.Defined());
        Zone->GetShmState().Settings = *ToInitSettings;
    }

    // static
    inline int TShmCache::RBTCompare(ui32 aCrc32, ui32 aKeyLength, ui8* aKeyPtr, ui32 bCrc32, ui32 bKeyLength, ui8* bKeyPtr) {
        if (aCrc32 < bCrc32) {
            return -1;
        } else if (aCrc32 > bCrc32) {
            return 1;
        } else if (aKeyLength < bKeyLength) {
            return -1;
        } else if (aKeyLength > bKeyLength) {
            return 1;
        } else {
            return std::memcmp(aKeyPtr, bKeyPtr, aKeyLength);
        }
    }

    // static
    void TShmCache::RBTreeInsertValue(ngx_rbtree_node_t* temp, ngx_rbtree_node_t* node, ngx_rbtree_node_t* sentinel) {
        ngx_rbtree_node_t** p;

        while (true) {
            TElement& nodeElt = *Field2Elt<ngx_rbtree_node_t, &TElement::RBTreeNode>(node);
            TElement& tempElt = *Field2Elt<ngx_rbtree_node_t, &TElement::RBTreeNode>(temp);

            p = (RBTCompare(node->key, nodeElt.KeyLength, nodeElt.KeyPtr(), temp->key, tempElt.KeyLength, tempElt.KeyPtr()) < 0) ? &temp->left : &temp->right;

            if (*p == sentinel) {
                break;
            }

            temp = *p;
        }

        *p = node;
        node->parent = temp;
        node->left = sentinel;
        node->right = sentinel;
        ngx_rbt_red(node);
    }

    TShmCache::TElement* TShmCache::FindElement(const ui32 crc32, const TString& key) {
        TState& state = Zone->GetShmState();
        ngx_rbtree_node_t* node = state.RBTree.root;
        ngx_rbtree_node_t* const sentinel = state.RBTree.sentinel;

        while (node != sentinel) {
            TElement* nodeElt = Field2Elt<ngx_rbtree_node_t, &TElement::RBTreeNode>(node);

            const int cmpr = RBTCompare(crc32, key.length(), (ui8*)key.data(), node->key, nodeElt->KeyLength, nodeElt->KeyPtr());

            if (cmpr < 0) {
                node = node->left;
            } else if (cmpr > 0) {
                node = node->right;
            } else {
                return nodeElt;
            }
        }

        return nullptr;
    }

    void TShmCache::RemoveElement(TElement* element) {
        ngx_rbtree_delete(&Zone->GetShmState().RBTree, &element->RBTreeNode);
        ngx_queue_remove(&element->QMainNode);
        Zone->Free((ui8*)element);
    }

    void TShmCache::RemoveExpiredElements() {
        const ngx_msec_t now = ngx_current_msec;

        TState& state = Zone->GetShmState();

        for (ngx_queue_t* queue : {&state.QReady, &state.QInProgress}) {
            while (!ngx_queue_empty(queue)) {
                ngx_queue_t* head = ngx_queue_head(queue);
                TElement* headElt = Field2Elt<ngx_queue_t, &TElement::QMainNode>(head);
                const ngx_msec_t expireTime = headElt->CreationTime + (headElt->DataSize > 0 ? state.Settings.ReadyTimeout : state.Settings.InProgressTimeout);

                if (ngx_msec_int_t(now - expireTime) >= 0) {
                    RemoveElement(headElt);
                } else {
                    break;
                }
            }
        }
    }

    bool TShmCache::RemoveOneOldestElement() {
        TState& state = Zone->GetShmState();

        ngx_queue_t* queue = &state.QReady;
        if (ngx_queue_empty(queue)) {
            return false;
        }

        ngx_queue_t* head = ngx_queue_head(queue);
        TElement* headElt = Field2Elt<ngx_queue_t, &TElement::QMainNode>(head);
        RemoveElement(headElt);

        return true;
    }

    namespace {
        TBuffer LoaderTBuffer(void const* data, size_t size) {
            return TBuffer((char const*)data, size);
        }

        const TBuffer& SaverTBuffer(const TBuffer& data) {
            return data;
        }
    }

    NThreading::TFuture<TBuffer> TShmCache::Get(
        TRequestWorker& request,
        const TString& key,
        TDataGetter<TBuffer> runGetter) {
        return Get<TBuffer, TBuffer, const TBuffer&>(request, key, runGetter, LoaderTBuffer, SaverTBuffer);
    }

    TShmCache::TInProgressElementCleaner::TInProgressElementCleaner(
        TShmCache& cache,
        TRequestWorker* requestPtr,
        const ui32 crc32,
        TString key)
        : Cache(cache)
        , RequestPtr(requestPtr)
        , Crc32(crc32)
        , Key(std::move(key))
        , Triggered(false)
    {
    }

    void TShmCache::TInProgressElementCleaner::Trigger() {
        if (!Triggered) {
            const TShmMutex shmMutex = Cache.Zone->GetMutex();
            TGuard<TShmMutex> shmGuard(shmMutex);

            Triggered = true;

            auto element = Cache.FindElement(Crc32, Key);
            // If in-progress element created by associated request is present, it shall be removed from cache
            if (element && element->DataSize == 0 && element->RequestPtr == RequestPtr && element->Pid == ngx_getpid()) {
                Cache.RemoveElement(element);
            }
        }
    }

    TShmCache::TInProgressElementCleaner::~TInProgressElementCleaner() {
        Trigger();
    }
}
