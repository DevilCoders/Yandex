#pragma once

/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.
   Copyright 2017-2018 YANDEX LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// An optimization pass that inserts MkldnnToTf conversion nodes in the graph

#if defined(ARCADIA_BUILD_ROOT)

#include <sys/types.h>
#include <memory>
#include "tensorflow/core/graph/graph.h"

namespace tensorflow {
    // Interface to invoke the pass for unit test
    //
    // Returns true if and only if 'g' is mutated.
    extern bool InsertMkldnnToTfConversionNodes(std::unique_ptr<Graph>* g);
}

#endif /* ARCADIA_BUILD_ROOT */
