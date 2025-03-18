#pragma once

#include <util/random/mersenne.h>

#include <utility>

template <typename TNode>
class TRandomPointGenerator {
public:
    ui64 NextPoint() {
        return PointRnd.GenRand();
    }

    void NextNode(const TNode&) {
    }

private:
    TMersenne<ui64> PointRnd;
};

template <typename TNode, typename TPoint>
class TPointGeneratorBase {
public:
    virtual ~TPointGeneratorBase() = default;

    virtual TPoint NextPoint() = 0;
    virtual void NextNode(const TNode&) = 0;
};

template <typename TNode, typename TPoint, typename TPointGenerator>
class TPointGeneratorModel: public TPointGeneratorBase<TNode, TPoint> {
public:
    explicit TPointGeneratorModel(TPointGenerator generator)
        : Generator(std::move(generator))
    {
    }

    TPoint NextPoint() override {
        return Generator.NextPoint();
    }

    void NextNode(const TNode& node) override {
        Generator.NextNode(node);
    }

private:
    TPointGenerator Generator;
};
