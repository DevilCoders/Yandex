#pragma once

template<typename TIteratorType, typename TValueType, typename TPredicate>
class TFilterIterator {
private:
    TIteratorType Current_;
    TIteratorType End_;
    TPredicate Predicate_;

public:
    TFilterIterator(const TIteratorType& begin, const TIteratorType& end, const TPredicate& predicate)
        : Current_(begin)
        , End_(end)
        , Predicate_(predicate)
    {
        while ((Current_ != End_) && !Predicate_(*Current_)) {
            ++Current_;
        }
    }

    TValueType& operator*() const {
        return *Current_;
    }

    TValueType* operator->() const {
        return Current_.operator->();
    }

    TFilterIterator& operator++() {
        Advance();

        return *this;
    }

    TFilterIterator operator++(int) {
        TFilterIterator tmp(*this);

        Advance();

        return tmp;
    }

    bool operator==(const TFilterIterator& it) const {
        return Current_ == it.Current_;
    }

    bool operator!=(const TFilterIterator& it) const {
        return Current_ != it.Current_;
    }

    TFilterIterator End() {
        return TFilterIterator(End_, End_, Predicate_);
    }

private:
    void Advance() {
        do {
            ++Current_;
        } while ((Current_ != End_) && !Predicate_(*Current_));
    }
};

template<typename TIteratorType, typename TValueType, typename TPredicate>
class TFilterConstIterator {
private:
    TIteratorType Current_;
    TIteratorType End_;
    TPredicate Predicate_;

public:
    TFilterConstIterator(const TIteratorType& begin, const TIteratorType& end, const TPredicate& predicate)
        : Current_(begin)
        , End_(end)
        , Predicate_(predicate)
    {
        while ((Current_ != End_) && !Predicate_(*Current_)) {
            ++Current_;
        }
    }

    const TValueType& operator*() const {
        return *Current_;
    }

    const TValueType* operator->() const {
        return Current_.operator->();
    }

    TFilterConstIterator& operator++() {
        Advance();

        return *this;
    }

    TFilterConstIterator operator++(int) {
        TFilterConstIterator tmp(*this);

        Advance();

        return tmp;
    }

    bool operator==(const TFilterConstIterator& it) const {
        return Current_ == it.Current_;
    }

    bool operator!=(const TFilterConstIterator& it) const {
        return Current_ != it.Current_;
    }

    TFilterConstIterator End() {
        return TFilterConstIterator(End_, End_, Predicate_);
    }

private:
    void Advance() {
        do {
            ++Current_;
        } while ((Current_ != End_) && !Predicate_(*Current_));
    }
};

template<typename TPredicate>
class TNotPredicate {
private:
    TPredicate Predicate;

public:
    TNotPredicate(const TPredicate& predicate)
        : Predicate(predicate)
    {
    }

    template<typename TItem>
    bool operator()(const TItem& item) {
        return !Predicate(item);
    }
};

