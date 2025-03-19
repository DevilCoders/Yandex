#pragma once

#include <cassert>
#include <util/system/defaults.h>
#include <util/generic/noncopyable.h>

enum {
    Portion_RawHitsFlag = 1, // Для сортировки данных, разбитых не по документам
    Portion_NoForms = 2,     // Не обрабатывать номера словоформ (временному файлу - YNDEX_VERSION_RAW64_HITS)
    Portion_UniqueHits = 4
};

// Порция ничего не знает о формате хранилища индекса
// и последовательно вызывает этот интерфейс при сохранении,
// гарантируя отсортированность
struct IYndexStorage
{
    enum FORMAT {
        FINAL_FORMAT        = 0,
        PORTION_FORMAT      = 1,
        PORTION_FORMAT_ATTR = 3,
        PORTION_FORMAT_LEMM = 4,
    };
    virtual ~IYndexStorage() {}
    virtual void StorePositions(const char *TextPointer, SUPERLONG *FilePositions, size_t FilePosCount) = 0;
};

struct IYndexStorageFactory
{
    ui32 Flags;
    IYndexStorageFactory() : Flags(0) {}
    virtual ~IYndexStorageFactory() {}

    // просит хранилище для сохранения текущей порции
    virtual void GetStorage(IYndexStorage **, IYndexStorage::FORMAT) = 0;

    // извещает, что сохранение завершено
    virtual void ReleaseStorage(IYndexStorage *) = 0;
};

class y_auto_storage : TNonCopyable {
public:
    y_auto_storage(IYndexStorageFactory* sf, IYndexStorage::FORMAT format)
      : Sf(sf)
    {
        assert(Sf);
        storage = nullptr;
        Sf->GetStorage(&storage, format);
    }
    ~y_auto_storage() {
        Sf->ReleaseStorage(storage);
    }
    IYndexStorage* operator->() const {
        return storage;
    }
    IYndexStorage& operator*() const {
        return *storage;
    }
private:
    IYndexStorageFactory* Sf;
    IYndexStorage* storage;
};
