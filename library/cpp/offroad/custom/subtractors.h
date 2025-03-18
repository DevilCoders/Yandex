#pragma once

#include <util/system/compiler.h>
#include <library/cpp/vec4/vec4.h>

namespace NOffroad {
    template <class Chunk>
    Y_FORCE_INLINE static void Integrate1(Chunk&& chunk0) {
        size_t i = 0;
        TVec4i accum;
        for (; i + 4 <= chunk0.size(); i += 4) {
            TVec4i curr(&chunk0[i]);
            curr = curr + curr.LeftShift<2>();
            curr = curr + curr.LeftShift<1>();
            curr = curr + accum.Shuffle<3, 3, 3, 3>();
            accum = curr;
            curr.Store(&chunk0[i]);
        }
        i += (i == 0);
        for (; i < chunk0.size(); ++i) {
            chunk0[i] += chunk0[i - 1];
        }
    }

    class TINSubtractor {
    public:
        enum {
            TupleSize = -1,
            PrefixSize = -1,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&&, Delta&& delta, Next&& next) {
            next.Assign(delta);
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&&, Next&& next, Delta&& delta) {
            delta.Assign(next);
        }
    };

    class TI1Subtractor {
    public:
        enum {
            TupleSize = 1,
            PrefixSize = 0,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&&, Delta&& delta, Next&& next) {
            next[0] = delta[0];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&&, Next&& next, Delta&& delta) {
            delta[0] = next[0];
        }
    };

    class TD1Subtractor {
    public:
        enum {
            TupleSize = 1,
            PrefixSize = 0,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            next[0] = value[0] + delta[0];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
        }
    };

    class TPD1Subtractor {
    public:
        enum {
            TupleSize = 1,
            PrefixSize = 1,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            Integrate1(storage.Chunk(0));
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&&, Delta&& delta, Next&& next) {
            next.Assign(delta);
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
        }
    };

    class TD1I1Subtractor {
    public:
        enum {
            TupleSize = 2,
            PrefixSize = 0,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            next[0] = value[0] + delta[0];
            next[1] = delta[1];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            delta[1] = next[1];
        }
    };

    class TD2Subtractor {
    public:
        enum {
            TupleSize = 2,
            PrefixSize = 0,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            next[0] = value[0] + delta[0];
            next[1] = delta[0] ? delta[1] : value[1] + delta[1];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            delta[1] = delta[0] ? next[1] : next[1] - value[1];
        }
    };

    class TD2I1Subtractor {
    public:
        enum {
            TupleSize = 3,
            PrefixSize = 0,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != 0) {
                next[0] = value[0] + delta[0];
                next[1] = delta[1];
                next[2] = delta[2];
                return;
            }

            next[0] = value[0];
            next[1] = value[1] + delta[1];
            next[2] = delta[2];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                return;
            }

            delta[1] = next[1] - value[1];
            delta[2] = next[2];
        }
    };

    class TD3I1Subtractor {
    public:
        enum {
            TupleSize = 4,
            PrefixSize = 0,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != 0) {
                next[0] = value[0] + delta[0];
                next[1] = delta[1];
                next[2] = delta[2];
                next[3] = delta[3];
                return;
            }
            next[0] = value[0];

            if (delta[1] != 0) {
                next[1] = value[1] + delta[1];
                next[2] = delta[2];
                next[3] = delta[3];
                return;
            }
            next[1] = value[1];

            next[2] = value[2] + delta[2];
            next[3] = delta[3];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                delta[3] = next[3];
                return;
            }

            delta[1] = next[1] - value[1];
            if (delta[1] != 0) {
                delta[2] = next[2];
                delta[3] = next[3];
                return;
            }

            delta[2] = next[2] - value[2];
            delta[3] = next[3];
        }
    };

    class TD4I1Subtractor {
    public:
        enum {
            TupleSize = 5,
            PrefixSize = 0,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != 0) {
                next[0] = value[0] + delta[0];
                next[1] = delta[1];
                next[2] = delta[2];
                next[3] = delta[3];
                next[4] = delta[4];
                return;
            }
            next[0] = value[0];

            if (delta[1] != 0) {
                next[1] = value[1] + delta[1];
                next[2] = delta[2];
                next[3] = delta[3];
                next[4] = delta[4];
                return;
            }
            next[1] = value[1];

            if (delta[2] != 0) {
                next[2] = value[2] + delta[2];
                next[3] = delta[3];
                next[4] = delta[4];
                return;
            }
            next[2] = value[2];

            next[3] = value[3] + delta[3];
            next[4] = delta[4];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                delta[3] = next[3];
                delta[4] = next[4];
                return;
            }

            delta[1] = next[1] - value[1];
            if (delta[1] != 0) {
                delta[2] = next[2];
                delta[3] = next[3];
                delta[4] = next[4];
                return;
            }

            delta[2] = next[2] - value[2];
            if (delta[2] != 0) {
                delta[3] = next[3];
                delta[4] = next[4];
                return;
            }

            delta[3] = next[3] - value[3];
            delta[4] = next[4];
        }
    };

    class TPD1I1Subtractor {
    public:
        enum {
            TupleSize = 2,
            PrefixSize = 1,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            Integrate1(storage.Chunk(0));
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&&, Delta&& delta, Next&& next) {
            next.Assign(delta);
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            delta[1] = next[1];
        }
    };

    class TPD1D1Subtractor {
    public:
        enum {
            TupleSize = 2,
            PrefixSize = 1,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            Integrate1(storage.Chunk(0));
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != value[0]) {
                next.Assign(delta);
                return;
            }
            next[0] = value[0];

            next[1] = value[1] + delta[1];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                return;
            }

            delta[1] = next[1] - value[1];
        }
    };

    class TPD1D2Subtractor {
    public:
        enum {
            TupleSize = 3,
            PrefixSize = 1,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            Integrate1(storage.Chunk(0));
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != 0) {
                next[0] = value[0] + delta[0];
                next[1] = delta[1];
                next[2] = delta[2];
                return;
            }

            next[0] = value[0];
            next[1] = value[1] + delta[1];
            next[2] = delta[2];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                return;
            }

            delta[1] = next[1] - value[1];
            delta[2] = next[2];
        }
    };

    class TPD1D2I1Subtractor {
    public:
        enum {
            TupleSize = 4,
            PrefixSize = 1,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            Integrate1(storage.Chunk(0));
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != value[0]) {
                next.Assign(delta);
                return;
            }

            if (delta[1] != 0) {
                next[1] = value[1] + delta[1];
                next[2] = delta[2];
                next[3] = delta[3];
                return;
            }
            next[1] = value[1];

            next[2] = value[2] + delta[2];
            next[3] = delta[3];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                delta[3] = next[3];
                return;
            }

            delta[1] = next[1] - value[1];
            if (delta[1] != 0) {
                delta[2] = next[2];
                delta[3] = next[3];
                return;
            }

            delta[2] = next[2] - value[2];
            delta[3] = next[3];
        }
    };

    class TPD1D3I1Subtractor {
    public:
        enum {
            TupleSize = 5,
            PrefixSize = 1,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            Integrate1(storage.Chunk(0));
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != value[0]) {
                next.Assign(delta);
                return;
            }

            if (delta[1] != 0) {
                next[1] = value[1] + delta[1];
                next[2] = delta[2];
                next[3] = delta[3];
                next[4] = delta[4];
                return;
            }
            next[1] = value[1];

            if (delta[2] != 0) {
                next[2] = value[2] + delta[2];
                next[3] = delta[3];
                next[4] = delta[4];
                return;
            }
            next[2] = value[2];

            next[3] = value[3] + delta[3];
            next[4] = delta[4];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                delta[3] = next[3];
                delta[4] = next[4];
                return;
            }

            delta[1] = next[1] - value[1];
            if (delta[1] != 0) {
                delta[2] = next[2];
                delta[3] = next[3];
                delta[4] = next[4];
                return;
            }

            delta[2] = next[2] - value[2];
            if (delta[2] != 0) {
                delta[3] = next[3];
                delta[4] = next[4];
                return;
            }

            delta[3] = next[3] - value[3];
            delta[4] = next[4];
        }
    };

    class TPD2D3I1Subtractor {
    public:
        enum {
            TupleSize = 6,
            PrefixSize = 2,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            auto chunk0 = storage.Chunk(0);
            auto chunk1 = storage.Chunk(1);

            for (size_t i = 1; i < chunk0.size(); i++) {
                if (chunk0[i] == 0)
                    chunk1[i] += chunk1[i - 1];
                chunk0[i] += chunk0[i - 1];
            }
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != value[0] || delta[1] != value[1]) {
                next.Assign(delta);
                return;
            }

            if (delta[2] != 0) {
                next[2] = value[2] + delta[2];
                next[3] = delta[3];
                next[4] = delta[4];
                next[5] = delta[5];
                return;
            }
            next[2] = value[2];

            if (delta[3] != 0) {
                next[3] = value[3] + delta[3];
                next[4] = delta[4];
                next[5] = delta[5];
                return;
            }
            next[3] = value[3];

            next[4] = value[4] + delta[4];
            next[5] = delta[5];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                delta[3] = next[3];
                delta[4] = next[4];
                delta[5] = next[5];
                return;
            }

            delta[1] = next[1] - value[1];
            if (delta[1] != 0) {
                delta[2] = next[2];
                delta[3] = next[3];
                delta[4] = next[4];
                delta[5] = next[5];
                return;
            }

            delta[2] = next[2] - value[2];
            if (delta[2] != 0) {
                delta[3] = next[3];
                delta[4] = next[4];
                delta[5] = next[5];
                return;
            }

            delta[3] = next[3] - value[3];
            if (delta[3] != 0) {
                delta[4] = next[4];
                delta[5] = next[5];
                return;
            }

            delta[4] = next[4] - value[4];
            delta[5] = next[5];
        }
    };

    class TPD2D1I1Subtractor {
    public:
        enum {
            TupleSize = 4,
            PrefixSize = 2,
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&& storage) {
            auto chunk0 = storage.Chunk(0);
            auto chunk1 = storage.Chunk(1);

            for (size_t i = 1; i < chunk0.size(); i++) {
                if (chunk0[i] == 0)
                    chunk1[i] += chunk1[i - 1];
                chunk0[i] += chunk0[i - 1];
            }
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (delta[0] != value[0] || delta[1] != value[1]) {
                next.Assign(delta);
                return;
            }

            next[0] = value[0];
            next[1] = value[1];
            next[2] = value[2] + delta[2];
            next[3] = delta[3];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0] != 0) {
                delta[1] = next[1];
                delta[2] = next[2];
                delta[3] = next[3];
                return;
            }

            delta[1] = next[1] - value[1];
            if (delta[1] != 0) {
                delta[2] = next[2];
                delta[3] = next[3];
                return;
            }

            delta[2] = next[2] - value[2];
            delta[3] = next[3];
        }
    };
}
