#pragma once

#include "policy.h"

#include <library/cpp/logger/global/global.h>

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/set.h>

template <class TCoord, class TObj>
class TRectBuilderDefault {
public:
    TRect<TCoord> GetRect(const TObj& obj) const {
        return obj.GetRect();
    }
};

template <class TCoord, class TObj>
class TRectBuilderDefaultPtr {
public:
    TRect<TCoord> GetRect(const TObj& obj) const {
        return obj->GetRect();
    }
};

template <class TRectBuilder, class TBuilderPolicy>
class TRectHashFeatures {
protected:
    TRectBuilder RectBuilder;
    TBuilderPolicy BuilderPolicy;

public:
    mutable ui32 Counter = 0;

public:
    TRectHashFeatures(const TRectBuilder& rectBuilder = TRectBuilder(),
        const TBuilderPolicy builderPolicy = TBuilderPolicy())
        : RectBuilder(rectBuilder)
        , BuilderPolicy(builderPolicy)
    {
    }

    Y_FORCE_INLINE const TRectBuilder& GetRectBuilder() const {
        return RectBuilder;
    }

    Y_FORCE_INLINE const TBuilderPolicy& GetBuilderPolicy() const {
        return BuilderPolicy;
    }
};

template <class TCoord, class TObj,
    class TRectBuilder = TRectBuilderDefault<TCoord, TObj>,
    class TBuilderPolicy = THashBuilderPolicyDefault<TCoord, TObj>,
    ui32 SizeDivider = 2
>
    class TRectHashNode {
    private:
        using TRect = ::TRect<TCoord>;
        using TSelf = TRectHashNode<TCoord, TObj, TRectBuilder, TBuilderPolicy>;
        using TBaseFeatures = TRectHashFeatures<TRectBuilder, TBuilderPolicy>;

    private:
        TRect RectMain;
        const ui32 Id;
        TVector<TSelf> Slaves;
        TVector<TObj> Objects;
        ui32 ElementsCount = 0;

    public:
        const TRect& GetRect() const {
            return RectMain;
        }

        const TVector<TSelf>& GetSlaves() const {
            return Slaves;
        }

        using TObject = TObj;

        TRectHashNode(const TRect& rectMain, const ui32 id)
            : RectMain(rectMain)
            , Id(id)
        {
            Slaves.reserve(SizeDivider * SizeDivider);
        }

        void Clear() {
            Slaves.clear();
            Objects.clear();
        }

        void Print(IOutputStream& stream, const ui32 level = 0) const {
            if (Slaves.size()) {
                stream << TString(level, '-') << RectMain.ToString() << Endl;
                for (auto&& slave : Slaves) {
                    slave.Print(stream, level + 1);
                }
            } else {
                stream << TString(level, '-') << RectMain.ToString() << "/" << Objects.size() << Endl;
            }
        }

        template <class TActor>
        Y_FORCE_INLINE bool FindObjects(const TRect& rect, TActor& actor, const TBaseFeatures& hashInfo, TSet<ui32>* visited = nullptr) const {
            return FindObjectsImpl(rect, rect, actor, hashInfo, visited);
        }

        template <class TActor>
        Y_FORCE_INLINE bool FindObjectsImpl(const TRect& rect, const TRect& rectFindObject, TActor& actor, const TBaseFeatures& hashInfo, TSet<ui32>* visited = nullptr) const {
            if (!RectMain.Cross(rect)) {
                return false;
            }
            if (!Slaves.empty()) {
                bool result = false;
                for (auto&& i : Slaves) {
                    result |= i.FindObjectsImpl(rect, rectFindObject, actor, hashInfo, visited);
                }
                return result;
            } else {
                if (visited) {
                    if (visited->contains(Id)) {
                        return false;
                    }
                    visited->insert(Id);
                }
                for (auto&& obj : Objects) {
                    if (rectFindObject.Cross(hashInfo.GetRectBuilder().GetRect(obj))) {
                        if (!actor(obj))
                            break;
                    }
                }
                return true;
            }
        }

        ui32 Size() const {
            return ElementsCount;
        }

        const TSelf& GetBottomNode(const TRect& rect) const {
            if (!Slaves.empty()) {
                for (auto&& i : Slaves) {
                    if (i.RectMain.ContainLB(rect)) {
                        return i.GetBottomNode(rect);
                    }
                }
            }
            return *this;
        }

        void CutForRect(const TRect& rect) {
            TRect startRect = RectMain;
            bool changed;
            ui32 counter = 0;
            do {
                CHECK_WITH_LOG(startRect.ContainLB(rect));
                changed = false;
                TRect newMain;
                for (ui32 divX = 0; !changed && divX < SizeDivider; ++divX) {
                    double multMinX = 1.0 * divX / SizeDivider;
                    double multMaxX = 1.0 * (divX + 1) / SizeDivider;
                    for (ui32 divY = 0; !changed && divY < SizeDivider; ++divY) {
                        double multMinY = 1.0 * divY / SizeDivider;
                        double multMaxY = 1.0 * (divY + 1) / SizeDivider;
                        newMain.Min.X = startRect.Max.X * multMinX + startRect.Min.X * (1 - multMinX);
                        newMain.Max.X = startRect.Max.X * multMaxX + startRect.Min.X * (1 - multMaxX);

                        newMain.Min.Y = startRect.Max.Y * multMinY + startRect.Min.Y * (1 - multMinY);
                        newMain.Max.Y = startRect.Max.Y * multMaxY + startRect.Min.Y * (1 - multMaxY);
                        if (newMain.ContainLB(rect)) {
                            changed = true;
                            startRect = newMain;
                            ++counter;
                            break;
                        }
                    }
                }
            } while (changed);
            INFO_LOG << "Iterators RectHash reduced by " << counter << Endl;
            if (counter) {
                TVector<TRect> rects = startRect.GetAdditionalRects(RectMain);
                rects.push_back(startRect);
            }
        }

        ui32 SlavesSize() const {
            return Slaves.size();
        }

        template <class TActor>
        void Scan(TActor& actor) const {
            if (Slaves.size()) {
                if (actor.OnBigRect(RectMain, *this)) {
                    for (auto&& slave : Slaves) {
                        slave.Scan(actor);
                    }
                }
            } else {
                if (actor.OnRect(RectMain, Objects)) {
                    for (auto&& i : Objects) {
                        actor.OnObject(i);
                    }
                }
            }
        }

        template <class TActor>
        void ScanLambda(TActor& actor) const {
            if (Slaves.size()) {
                for (auto&& slave : Slaves) {
                    slave.ScanLambda(actor);
                }
            } else {
                for (auto&& i : Objects) {
                    actor(i);
                }
            }
        }

        TVector<TObj> GetObjects(bool skipDuplicates = false) const {
            TVector<TObj> objectsSet;
            const auto actor = [&objectsSet](const TObj& obj) { objectsSet.push_back(obj); };
            ScanLambda(actor);

            if (skipDuplicates)
                return objectsSet;

            Sort(objectsSet.begin(), objectsSet.end());

            TVector<TObj> result;
            for (ui32 i = 0; i < objectsSet.size(); ++i) {
                if (result.size() == 0) {
                    result.push_back(objectsSet[i]);
                }

                if (!(result.back() == objectsSet[i])) {
                    result.push_back(objectsSet[i]);
                }
            }

            return result;
        }

        bool Remove(const TObj& object, const TBaseFeatures& hashInfo) {
            if (!RectMain.Cross(hashInfo.GetRectBuilder().GetRect(object))) {
                return false;
            }

            bool result = false;

            if (Slaves.size()) {
                for (auto&& i : Slaves) {
                    result |= i.Remove(object, hashInfo);
                }
            } else {
                for (ui32 i = 0; i < Objects.size(); ++i) {
                    if (Objects[i] == object) {
                        Objects.erase(Objects.begin() + i);
                        result = true;
                        break;
                    }
                }
            }

            if (result) {
                --ElementsCount;
                DoNormalization(hashInfo);
            }
            return result;
        }

        bool AddObject(const TObj& object, const TBaseFeatures& hashInfo) {
            if (!RectMain.CrossLB(hashInfo.GetRectBuilder().GetRect(object))) {
                return false;
            }

            ++ElementsCount;
            bool result = false;
            if (Slaves.size()) {
                for (auto&& i : Slaves) {
                    result |= i.AddObject(object, hashInfo);
                }
            } else {
                Objects.push_back(object);
                result = true;
            }

            DoNormalization(hashInfo);
            return result;
        }

        void Normalization(const TBaseFeatures& hashInfo) {
            for (auto&& slave : Slaves) {
                slave.Normalization(hashInfo);
            }
            DoNormalization(hashInfo);
        }

    private:
        void DoNormalization(const TBaseFeatures& hashInfo) {
            if (IsObjectContainer()) {
                if (hashInfo.GetBuilderPolicy().CheckSplitCondition(RectMain, Objects)) {
                    TRect newMain;
                    for (ui32 divX = 0; divX < SizeDivider; ++divX) {
                        double multMinX = 1.0 * divX / SizeDivider;
                        double multMaxX = 1.0 * (divX + 1) / SizeDivider;
                        for (ui32 divY = 0; divY < SizeDivider; ++divY) {
                            double multMinY = 1.0 * divY / SizeDivider;
                            double multMaxY = 1.0 * (divY + 1) / SizeDivider;
                            newMain.Min.X = RectMain.Max.X * multMinX + RectMain.Min.X * (1 - multMinX);
                            newMain.Max.X = RectMain.Max.X * multMaxX + RectMain.Min.X * (1 - multMaxX);

                            newMain.Min.Y = RectMain.Max.Y * multMinY + RectMain.Min.Y * (1 - multMinY);
                            newMain.Max.Y = RectMain.Max.Y * multMaxY + RectMain.Min.Y * (1 - multMaxY);
                            Slaves.emplace_back(newMain, ++hashInfo.Counter);
                        }
                    }
                    for (ui32 i = 0; i < Objects.size(); ++i) {
                        bool found = false;
                        for (ui32 slave = 0; slave < Slaves.size(); ++slave) {
                            if (Slaves[slave].AddObject(Objects[i], hashInfo)) {
                                found = true;
                            }
                        }
                        CHECK_WITH_LOG(found);
                    }
                    Objects.clear();
                }

            } else if (hashInfo.GetBuilderPolicy().GetGlueNecessary()) {
                bool checkGlueCond = true;
                for (auto&& slave : Slaves) {
                    if (!slave.IsObjectContainer() || slave.Size() == 0) {
                        checkGlueCond = false;
                        break;
                    }
                }

                if (checkGlueCond) {
                    TVector<TObj> objects = GetObjects(true);
                    if (hashInfo.GetBuilderPolicy().CheckGlueCondition(RectMain, objects)) {
                        CHECK_WITH_LOG(!Objects.size());
                        Objects = objects;
                        Slaves.clear();
                    }
                }

            }

            CHECK_WITH_LOG(Slaves.empty() || Objects.empty());
        }

        bool IsObjectContainer() {
            return Slaves.empty();
        }
    };

template <class TCoord, class TObj,
    class TRectBuilder = TRectBuilderDefault<TCoord, TObj>,
    class TBuilderPolicy = THashBuilderPolicyDefault<TCoord, TObj>, ui32 SizeDivider = 2>
class TRectHash: public TRectHashFeatures<TRectBuilder, TBuilderPolicy> {
public:
    using TNode = TRectHashNode<TCoord, TObj, TRectBuilder, TBuilderPolicy, SizeDivider>;
private:
    using TRect = ::TRect<TCoord>;
    using TSelf = TRectHash<TCoord, TObj, TRectBuilder, TBuilderPolicy, SizeDivider>;
    using TBase = TRectHashFeatures<TRectBuilder, TBuilderPolicy>;

private:
    TNode Root;

public:
    TRectHash(const TRect& rectMain,
        const TRectBuilder& rectBuilder = TRectBuilder(),
        const TBuilderPolicy builderPolicy = TBuilderPolicy())
        : TBase(rectBuilder, builderPolicy)
        , Root(rectMain, ++TBase::Counter)
    {
    }

    const TNode& GetBottomNode(const TRect& rect) const {
        return Root.GetBottomNode(rect);
    }

    void CutForRect(const TRect& rect) {
        CHECK_WITH_LOG(Root.GetObjects().size() == 0);
        CHECK_WITH_LOG(Root.SlavesSize() == 0);
        Root.CutForRect(rect);
    }

    void Clear() {
        Root.Clear();
    }

    ui32 Size() const {
        return Root.Size();
    }

    bool FindObjects(const TRect& rect, TSet<TObj>& objects) const {
        objects.clear();
        const auto actor = [&objects](const TObj& obj)->bool {
            objects.insert(obj);
            return true;
        };
        return FindObjects(rect, actor);
    }

    template <class TActor>
    bool FindObjects(const TRect& rect, TActor& actor) const {
        return Root.FindObjects(rect, actor, *this);
    }

    bool AddObject(const TObj& object) {
        return Root.AddObject(object, *this);
    }

    bool Remove(const TObj& object) {
        return Root.Remove(object, *this);
    }

    template <class TActor>
    void Scan(TActor& actor) const {
        Root.Scan(actor);
    }

    template <class TActor>
    void ScanLambda(TActor& actor) const {
        Root.ScanLambda(actor);
    }

    void Normalization() {
        Root.Normalization(*this);
    }

    void Print(IOutputStream& stream, const ui32 level = 0) const {
        Root.Print(stream, level);
    }

    TPolicyGuard<TSelf, TBuilderPolicy> GetPolicy() {
        return TPolicyGuard<TSelf, TBuilderPolicy>(*this, TBase::BuilderPolicy);
    }

    TVector<TObj> GetObjects(bool skipDuplicates = false) const {
        return Root.GetObjects(skipDuplicates);
    }

    const TObj* GetObject(const TCoord& coordinate) const {
        const TObj* result = nullptr;
        auto actor = [&result](const TObj& object) {
            if (!result) {
                result = &object;
            }
            return false;
        };
        FindObjects({ coordinate }, actor);
        return result;
    }
};
