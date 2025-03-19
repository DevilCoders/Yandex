#pragma once

#include <util/generic/deque.h>
#include <util/generic/stack.h>
#include <util/generic/ylimits.h>

#include <util/system/yassert.h>

namespace NTextMachine {
namespace NCore {
    // The following class implements
    // object stream with additional tree-like
    // structure on top of it.
    // For example A (B (C D) E F).
    //
    // * Can be written via TWriter, and read via TReader.
    // * While reading, tree structure should be respected.
    // * One child (contents of "(...)") can be read
    //   repeatedly several times using VisitChild method.
    //
    // Used to gather/broadcast data from nodes of text_machine.
    //
    template <typename T>
    class TStructuralStream {
    private:
        enum EStepType {
            StOpenChild = 0,
            StCloseChild = 1,
            StDataItem = 2
        };

        struct TOpenChildInfo {
            ui32 Handle;
            ui32 Length;
        };

        struct TCloseChildInfo {
            ui32 NumItems;
        };

        struct TDataItemInfo {
            const T* ItemPtr;
        };

        class TStepInfo {
            EStepType Type = StOpenChild;

            union {
                TOpenChildInfo OpenChildPart;
                TCloseChildInfo CloseChildPart;
                TDataItemInfo DataItemPart;
            } Select;

        public:
            TStepInfo() {
                memset(&Select, 0, sizeof(Select));
            }
            EStepType GetType() const {
                return Type;
            }

            void MakeOpenChild(size_t handle) {
                Type = StOpenChild;
                Select.OpenChildPart.Handle = handle;
            }
            TOpenChildInfo& OpenChild() {
                Y_ASSERT(StOpenChild == Type);
                return Select.OpenChildPart;
            }
            const TOpenChildInfo& GetOpenChild() const {
                Y_ASSERT(StOpenChild == Type);
                return Select.OpenChildPart;
            }

            void MakeCloseChild(size_t numItems) {
                Type = StCloseChild;
                Select.CloseChildPart.NumItems = numItems;
            }
            TCloseChildInfo& CloseChild() {
                Y_ASSERT(StCloseChild == Type);
                return Select.CloseChild;
            }
            const TCloseChildInfo& GetCloseChild() const {
                Y_ASSERT(StCloseChild == Type);
                return Select.CloseChildPart;
            }

            void MakeDataItem(const T* itemPtr) {
                Type = StDataItem;
                Select.DataItemPart.ItemPtr = itemPtr;
            }
            TDataItemInfo& DataItem() {
                Y_ASSERT(StDataItem == Type);
                return Select.DataItemPart;
            }
            const TDataItemInfo& GetDataItem() const {
                Y_ASSERT(StDataItem == Type);
                return Select.DataItemPart;
            }
        };

        struct TStreamData {
            TDeque<T> Items;
            TDeque<TStepInfo> Steps;
            ui32 NextChildHandle = ::Max<ui32>();
        };

        TStreamData Data;

    public:
        using TConstIterator = typename TDeque<T>::const_iterator;

        template <typename AccessorType>
        class TGuard {
            ui32 Handle = 0;
            AccessorType* Accessor = nullptr;

        public:
            TGuard(AccessorType& accessor)
                : Handle(accessor.OpenChild())
                , Accessor(&accessor)
            {}
            TGuard(AccessorType& accessor, size_t handle)
                : Handle(handle)
                , Accessor(&accessor)
            {
                Accessor->OpenChild(handle);
            }
            TGuard(TGuard<AccessorType>&& other)
                : Handle(other.Handle)
                , Accessor(other.Accessor)
            {
                other.Accessor = nullptr;
            }

            TGuard(const TGuard<AccessorType>&) = delete;
            TGuard& operator = (const TGuard<AccessorType>&) = delete;

            ~TGuard() {
                if (Accessor) {
                    Accessor->CloseChild(Handle);
                }
            }

            ui32 GetHandle() const {
                return Handle;
            }
            void VisitChild() const {
                Y_ASSERT(Accessor);
                Accessor->VisitChild(Handle);
            }
        };

        class TVisitorBase {
        public:
            void OnOpenChild(size_t /*handle*/) const {}
            void OnCloseChild(size_t /*handle*/) const {}
            void OnDataItem(const T& /*value*/, size_t /*index*/) const {}
        };

        template <typename WriterType>
        class TCopierBase
            : public TVisitorBase
        {
        protected:
            WriterType& Writer;

        public:
            TCopierBase(WriterType& writer)
                : Writer(writer)
            {}

            void OnOpenChild(size_t handle) const {
                Writer.OpenChild(handle);
            }
            void OnCloseChild(size_t handle) const {
                Writer.CloseChild(handle);
            }
        };

        template <typename WriterType>
        class TCopier
            : public TCopierBase<WriterType>
        {
        public:
            TCopier(WriterType& writer)
                : TCopierBase<WriterType>(writer)
            {}

            void OnDataItem(const T& value, size_t) const {
                TCopierBase<WriterType>::Writer.Next() = value;
            }
        };

        template <typename WriterType, typename FuncType>
        class TTransformer
            : public TCopierBase<WriterType>
        {
            FuncType Func;

        public:
            TTransformer(WriterType& writer, const FuncType& func)
                : TCopierBase<WriterType>(writer)
                , Func(func)
            {}

            void OnDataItem(const T& value, size_t index) const {
                Func(TCopierBase<WriterType>::Writer, value, index);
            }
        };

        class TWriter {
        public:
            size_t OpenChild();
            void OpenChild(size_t handle);
            void CloseChild(size_t handle);

            T& Next();
            T& Next(i64& index, bool isIntCounter = false);
            i64 GetIndex() const;

            TGuard<TWriter> Guard() {
                return {*this};
            }
            TGuard<TWriter> Guard(size_t handle) {
                return {*this, handle};
            }


        private:
            friend class TStructuralStream;

            TWriter(TStreamData& data)
                : Data(&data)
            {}

            TStreamData* Data;
            TStack<size_t> PosStack;
            TStack<size_t> NumStack;
            i64 IntCount = 0;
            i64 FloatCount = 0;
        };

        class TReader {
        public:
            size_t SkipChild();
            size_t OpenChild();
            void OpenChild(size_t handle);
            void VisitChild(size_t handle);
            void CloseChild(size_t handle);

            const T& Next();
            const T& Next(i64& index, bool isIntCounter = false);
            i64 GetIndex() const;

            template <typename VisitorType>
            bool ScanOneStep(VisitorType& visitor) {
                if (Pos >= Data->Steps.size()) {
                    return false;
                }
                const auto& step = Data->Steps[Pos];

                switch (step.GetType()) {
                    case StOpenChild: {
                        visitor.OnOpenChild(OpenChild());
                        break;
                    }
                    case StCloseChild: {
                        const size_t openPos = PosStack.top();
                        const auto& openStep = Data->Steps[openPos].GetOpenChild();
                        visitor.OnCloseChild(openStep.Handle);
                        CloseChild(openStep.Handle);
                        break;
                    }
                    case StDataItem: {
                        i64 index = 0;
                        const T& value = Next(index);
                        Y_ASSERT(index >= 0);
                        visitor.OnDataItem(value, index);
                        break;
                    }
                }

                return true;
            }

            TGuard<TReader> Guard() {
                return {*this};
            }
            TGuard<TReader> Guard(size_t handle) {
                return {*this, handle};
            }

        private:
            friend class TStructuralStream;

            TReader(const TStreamData& data)
                : Data(&data)
            {}

            const TStreamData* Data;
            size_t Pos = 0;
            TStack<size_t> PosStack;
            i64 Index = -1;
            i64 IntCount = 0;
            i64 FloatCount = 0;
        };

    public:
        TStructuralStream() = default;

        TWriter CreateWriter() {
            return TWriter(Data);
        }
        TReader CreateReader() const {
            return TReader(Data);
        }

        const T& operator [] (size_t index) const {
            return Data.Items[index];
        }
        T& operator [] (size_t index) {
            return Data.Items[index];
        }

        size_t NumItems() const {
            return Data.Items.size();
        }

        TConstIterator begin() const {
            return Data.Items.begin();
        }
        TConstIterator end() const {
            return Data.Items.end();
        }

        void Clear() {
            Data = TStreamData{};
        }

        void ReleaseContents(TDeque<T>& contents) {
            Data.Items.swap(contents);
            Clear();
        }

        template <typename VisitorType>
        void Scan(VisitorType& visitor) const {
            auto reader = CreateReader();
            while (reader.ScanOneStep(visitor)) {}
        }

        template <typename WriterType>
        void Copy(WriterType& writer) const {
            TCopier<WriterType> visitor(writer);
            Scan(visitor);
        }

        template <typename WriterType, typename FuncType>
        void CopyTransform(WriterType& writer, const FuncType& func) const {
            TTransformer<WriterType, FuncType> visitor(writer, func);
            Scan(visitor);
        }
    };

    template <typename T>
    inline size_t TStructuralStream<T>::TWriter::OpenChild() {
        OpenChild(Data->NextChildHandle);
        return Data->NextChildHandle--;
    }

    template <typename T>
    inline void TStructuralStream<T>::TWriter::OpenChild(size_t handle) {
        const size_t pos = Data->Steps.size();
        const size_t num = Data->Items.size();

        Data->Steps.emplace_back();
        Data->Steps.back().MakeOpenChild(handle);
        PosStack.push(pos);
        NumStack.push(num);
    }

    template <typename T>
    inline void TStructuralStream<T>::TWriter::CloseChild(size_t handle) {
        const size_t num = Data->Items.size();
        const size_t numToClose = NumStack.top();
        const size_t pos = Data->Steps.size();
        const size_t posToClose = PosStack.top();
        const size_t handleToClose = Data->Steps[posToClose].GetOpenChild().Handle;

        Y_ASSERT(handleToClose == handle);
        Y_ASSERT(num >= numToClose);
        Y_ASSERT(pos >= posToClose);

        Data->Steps.emplace_back();
        Data->Steps.back().MakeCloseChild(num - numToClose);
        Data->Steps[posToClose].OpenChild().Length = pos - posToClose;
        PosStack.pop();
        NumStack.pop();
    }

    template <typename T>
    inline T& TStructuralStream<T>::TWriter::Next() {
        Data->Items.emplace_back();
        Data->Steps.emplace_back();
        Data->Steps.back().MakeDataItem(&Data->Items.back());
        return Data->Items.back();
    }

    template <typename T>
    inline T& TStructuralStream<T>::TWriter::Next(i64& index, bool isIntCounter) {
        T& res = Next();
        index = isIntCounter ? IntCount++ : FloatCount++;
        return res;
    }

    template <typename T>
    i64 TStructuralStream<T>::TWriter::GetIndex() const {
        return static_cast<i64>(Data->Items.size()) - 1;
    }

    template <typename T>
    inline size_t TStructuralStream<T>::TReader::SkipChild() {
        const size_t handle = Data->Steps[Pos].GetOpenChild().Handle;

        Pos += Data->Steps[Pos].GetOpenChild().Length;
        Index += Data->Steps[Pos].GetCloseChild().NumItems;
        Pos += 1;
        return handle;
    }

    template <typename T>
    inline size_t TStructuralStream<T>::TReader::OpenChild() {
        const size_t handle = Data->Steps[Pos].GetOpenChild().Handle;
        OpenChild(handle);
        return handle;
    }

    template <typename T>
    inline void TStructuralStream<T>::TReader::OpenChild(size_t handle) {
        PosStack.push(Pos);
        Y_ASSERT(handle == Data->Steps[Pos].GetOpenChild().Handle);
        Pos += 1;
    }

    template <typename T>
    inline void TStructuralStream<T>::TReader::VisitChild(size_t handle) {
        Y_ASSERT(Pos > 0);

        // Calls directly following OpenChild are acceptable
        // and do nothing
        if (StOpenChild == Data->Steps[Pos - 1].GetType()) {
            Y_ASSERT(Data->Steps[Pos - 1].GetOpenChild().Handle == handle);
            return;
        }

        const size_t num = Data->Steps[Pos].GetCloseChild().NumItems;

        Y_ASSERT(Index >= -1 && num <= static_cast<size_t>(Index + 1));
        Index -= num;
        Pos = PosStack.top();
        Y_ASSERT(handle == Data->Steps[Pos].GetOpenChild().Handle);
        Pos += 1;
    }

    template <typename T>
    inline void TStructuralStream<T>::TReader::CloseChild(size_t handle) {
        Y_ASSERT(Pos > 0);

        if (StCloseChild == Data->Steps[Pos].GetType()) {
            const size_t openPos = PosStack.top();

            Y_ASSERT(handle == Data->Steps[openPos].GetOpenChild().Handle);
            Y_ASSERT(Pos == openPos + Data->Steps[openPos].GetOpenChild().Length);

            Pos += 1;
            PosStack.pop();
        } else if (StOpenChild == Data->Steps[Pos - 1].GetType()) {
            Y_ASSERT(Data->Steps[Pos - 1].GetOpenChild().Handle == handle);
            Pos -= 1;
            SkipChild();
        } else {
            Y_ASSERT(false);
        }
    }

    template <typename T>
    inline const T& TStructuralStream<T>::TReader::Next() {
        Y_ASSERT(!!Data->Steps[Pos].GetDataItem().ItemPtr);
        Index += 1;
        return *Data->Steps[Pos++].GetDataItem().ItemPtr;
    }

    template <typename T>
    inline const T& TStructuralStream<T>::TReader::Next(i64& index, bool isIntCounter) {
        const T& res = Next();
        index = isIntCounter ? IntCount++ : FloatCount++;
        return res;
    }

    template <typename T>
    inline i64 TStructuralStream<T>::TReader::GetIndex() const {
        return Index;
    }
} // NCore
} // NTextMachine
