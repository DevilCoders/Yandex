#pragma once

#include "attr_restrictions.h"

#include <util/generic/string.h>
#include <util/generic/hash_set.h>

#include <kernel/qtree/richrequest/richnode_fwd.h>


namespace NAttributes {

/*
    We make a tree of attributes. It is similar to qtree but it skips all the paths
    to non-attribute leaves. We can say that attribute tree is a boolean scheme with
    operations in nodes (And, Or, AndNot).

    The node consists of Left value (by default it is attribute value), Right value (only for
    segment iterators), indexes of Children, TreeOper operation of the tree (Or, And, AndNot, Leaf),
    index of future Iterator in filter, and CmpOper:
    (EQ for EQuality,
    LE for Less or Equal,
    LT for Less Than,
    GT for Greater Than,
    GE for Greater or Equal))
*/

TAttrRestrictions<> DecomposeAttrRestrictions(const TRichTreeConstPtr& richTree, bool useUTF8Attrs);


/*
    Validating AttrRestrictions
*/
bool IsValidAttrRestrictions(const TAttrRestrictions<>& attrRestriction);


} // namespace NAttributes
