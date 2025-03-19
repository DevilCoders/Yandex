#pragma once

#include <kernel/geodb/protos/geodb.pb.h>

#include <util/system/types.h>

namespace NGeoDB {
    using TEntityTypeWeight = i8;

    /** Return weight by entity type. Throw yexception in case there is no weight for type.
     * @see TryEntityTypeToWeight()
     * @param[in] type of entity for which you want a weight
     * @return weight
     */
    TEntityTypeWeight EntityTypeToWeight(const EType type);

    /** Find a weight for given entity type and put into weight.
     * @see EntityTypeToWeight()
     * @param[in] type of entity for which you want a weight
     * @param[out] weight is where it stores a found weight
     * @return true if weight for type is found, otherwise false
     */
    bool TryEntityTypeToWeight(const EType type, TEntityTypeWeight& weight);

} // namespace NGeoDB
