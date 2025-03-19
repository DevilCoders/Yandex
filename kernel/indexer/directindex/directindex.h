#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/direct_text/fl.h>
#include <kernel/indexer/direct_text/dt.h>
#include <kernel/indexer/dtcreator/dtcreator.h>
#include <kernel/tarc/iface/arcface.h>

class TArchiveCreator;
class IOutputStream;

namespace NIndexerCore {

class IDirectTextCallback2;
class TInvCreatorDTCallback;
class IRelevLevelCallback;
class TDefaultDisamber;

// Интерфейс для создания индекса напрямую, минуя парсер.
class TDirectIndex {
    template <typename T>
    struct TAttrImpl {
        typedef T TValue;

        TString Name;
        TValue Value;

        TAttrImpl() {
        }
        TAttrImpl(const TString& name, const TValue& value)
            : Name(name)
            , Value(value)
        {
        }
        bool operator == (const TAttrImpl& other) const {
            return Name == other.Name && Value == other.Value;
        }
        bool operator < (const TAttrImpl& other) const {
            return Name < other.Name || Name == other.Name && Value < other.Value;
        }
    };

public:
    typedef TAttrImpl<TString> TAttribute;
    typedef TAttrImpl<TUtf16String> TWAttribute;

    typedef TVector<TAttribute> TAttributes;
    typedef TVector<TWAttribute> TWAttributes;

    // @param backwardCompatible    if true - the old tokenization of marks will be used
    TDirectIndex(TDirectTextCreator& dtc, bool backwardCompatible = false);
    ~TDirectIndex();
    void Finish();
    // Вызываем один раз до первого AddDoc(), если нужно снимать леммную омонимию
    void SetDisamber(IDisambDirectText* obj);
    // Вызываем один раз до первого AddDoc(), если нужна модификация релевантностей на основе контекстов (после снятия омонимии)
    void SetRelevanceModificator(IModifyDirectText* obj);
    // Вызываем один раз до первого AddDoc(), если нужен инвертированный индекс
    void SetInvCreator(IDirectTextCallback4* obj);
    // Вызываем один раз до первого AddDoc(), если нужны текстовые архивы
    void SetArcCreator(TArchiveCreator* obj);
    // Вызываем по одному разу до первого AddDoc(), если нужно
    void AddDirectTextCallback(IDirectTextCallback2* obj);
    void AddDirectTextCallback(IDirectTextCallback5* obj);

    // Открывает индексирование документа.
    // Документ получает заданный внутренний идентификатор.
    // Устанавливает внутренний счетчик форм Sent.Word в 1.1
    void AddDoc(ui32 docId, ELanguage lang);

    // Закрывает процесс индексирования данного документа.
    // Если функция не вызвана, документ не попадает в индекс,
    // то есть все вызовы, начиная с последнего AddDoc() игнорируются.
    void CommitDoc(const TDocInfoEx* docInfo, TFullDocAttrs* docAttrs);

    // Добавляет в индекс документный поисковый литеральный атрибут.
    // Может быть вызвана в любой момент между AddDoc() и CommitDoc().
    // Не изменяет внутренний счетчик форм.
    void StoreDocAttr(const TAttribute & attr, EDTAttrType type = DTAttrSearchLiteral);
    void StoreDocAttr(const TWAttribute & attr, EDTAttrType type = DTAttrSearchLiteral);

    // Добавляет в индекс документный поисковый числовой атрибут.
    // Может быть вызвана в любой момент между AddDoc() и CommitDoc().
    // Не изменяет внутренний счетчик форм.
    void StoreDocIntegerAttr(const TWAttribute & attr);
    void StoreDocIntegerAttr(const TWAttribute & attr, TPosting pos);

    // Добавляет в индекс документный поисковый атрибут типа даты.
    // Может быть вызвана в любой момент между AddDoc() и CommitDoc().
    // Не изменяет внутренний счетчик форм.
    void StoreDocDateTimeAttr(const TString & name, time_t value);

    // Открывает зону в текущей позиции, задаваемой внутренним счетчиком форм.
    // Добавляет в этой позиции указанные зональные поисковые литеральные атрибуты.
    // Не изменяет внутренний счетчик форм.
    void OpenZone(const TString & zoneName);
    void OpenZone(const TString & zoneName, const TAttribute & zoneAttr);
    void OpenZone(const TString & zoneName, const TAttributes & zoneAttrs);
    void OpenZone(const TString & zoneName, const TWAttribute & zoneAttr);
    void OpenZone(const TString & zoneName, const TWAttributes & zoneAttrs);

    // Закрывает зону в текущей позиции, задаваемой внутренним счетчиком форм.
    // Не изменяет внутренний счетчик форм.
    // Число открытых и закрытых зон с данным именем должно быть одинаковым.
    void CloseZone(const TString & zoneName);

    // Добавляет в индекс все формы и леммы из указанного текста.
    // Текст токенизируется внутренним алгоритмом.
    // Формы лемматизируются указанными леммерами.
    // Каждая форма добавляется в текущей позиции, после чего
    // текущая позиция инкрементируется Sent.Word -> Sent.(Word+1), Sent.63 -> (Sent+1).1
    // В конце текста принудительно генерируется конец предложения.
    // relev - биты релевантности
    // sentAttrs - контейнер атрибутов, которые будут приписаны каждому предложению в архиве
    void StoreUtf8Text(const TString & text, RelevLevel relev);
    void StoreUtf8Text(const TString & text, RelevLevel relev, const TAttribute & sentAttr);
    void StoreUtf8Text(const TString & text, RelevLevel relev, const TAttributes & sentAttrs);

    void StoreText(const wchar16* text, size_t len, RelevLevel relev);
    void StoreText(const wchar16* text, size_t len, RelevLevel relev, const TAttribute & sentAttr);
    void StoreText(const wchar16* text, size_t len, RelevLevel relev, const TAttributes & sentAttrs);
    //! @param lemmerOptions    index of lemmer options in the portion
    void StoreText(const wchar16* text, size_t len, RelevLevel relev, const TLangMask& langMask, ELanguage lang, ui8 lemmerOptions, const TAttributes& sentAttrs, const IRelevLevelCallback* callback = nullptr, const void* callbackV5data = nullptr);

    // Устанавливает внутренний счетчик форм Sent.Word -> (Sent+k).1
    void IncBreak(ui32 k = 1);

    // Возвращает текущее значение Break
    ui16 CurrentBreak() const;

    // Устанавливает внутренний счетчик форм Sent.Word -> (Sent).(Word+1)
    void NextWord();

    // Возвращает текущую позицию в индексе.
    TWordPosition GetPosition() const;

    // Сколько уже было токенизировано форм.
    ui32 GetWordCount() const;

    // Игнорирование конца предложения от StoreText, а также внутри текста, переданного в StoreText
    void SetIgnoreStoreTextBreaks(bool val);
    bool GetIgnoreStoreTextBreaks() const;

    // Если true, вызовы StoreText никогда не приведут к увеличению номера предложения (лишний текст будет проигнорирован)
    // Желательно также выставить IgnoreStoreTextBreaks
    void SetNoImplicitBreaks(bool val);

    // Игнорирование любого увеличения словопозиции кроме прямого вызова NextWord()
    void SetIgnoreStoreTextNextWord(bool val);
    bool GetIgnoreStoreTextNextWord() const;

    // sets the maximum number of breaks on single StoreText() call
    void SetStoreTextMaxBreaks(ui32 val = BREAK_LEVEL_Max);

private:
    class TImpl;
    THolder<TImpl> Impl;
};

class TDirectIndexWithArchive : public TDirectIndex {
public:
    TDirectIndexWithArchive(TDirectTextCreator& dtc, const TString& textArchive, ui32 archiveVersion = ARCVERSION, bool backwardCompatible = false);
    TDirectIndexWithArchive(TDirectTextCreator& dtc, IOutputStream* textArchive, bool useArchiveHeader = true, ui32 archiveVersion = ARCVERSION);
    ~TDirectIndexWithArchive();

private:
    void Initialize(IOutputStream* archive, ui32 archiveVersion);

    ui32 GetArchiveCreatorSetting(ui32 archiveVersion);

    void SetupArcCreator(IOutputStream* archive, ui32 archiveCreatorSettings);

private:
    THolder<IOutputStream> OutArc;
    THolder<TArchiveCreator> PlainArchiveCreator;
};

}
