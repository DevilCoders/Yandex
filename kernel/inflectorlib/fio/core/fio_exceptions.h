#pragma once

#include <util/generic/yexception.h>


namespace NFioInflector {
    class TFioInflectorException: public yexception {
    };

    class TAnalyzeException: public TFioInflectorException {
    };

    class TGenerateException: public TFioInflectorException {
    };
}
