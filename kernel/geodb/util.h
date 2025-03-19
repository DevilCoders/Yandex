#pragma once

#include <util/generic/fwd.h>

namespace NGeoDB {
    class TGeoPtr;
}

namespace NGeoDB {
    /** Returns span as a string of given geo object.
     * It doesn't check if the data exist.
     * @param[in] geo is a geo object, if it is inavlid the function returns "0,0"
     * @param[in] widthFirst specify the order of width/height (see return value)
     * @return a string "$height,$width" if widthFirst false, otherwise "$width,$height"
     */
    TString SpanAsString(const TGeoPtr& geo, const bool widthFirst);

    /** Returns span as a string of given geo object in reverse order
     * It doesn't check if the data exists.
     * @see SpanAsString()
     * @param[in] geo is a geo object, if it is inavlid the function returns "0,0"
     * @return a string as "$height,$width"
     */
    TString SpanAsString(const TGeoPtr& geo);

    /** Returns location as a string of given geo object.
     * It doesn't check if the data exists.
     * @param[in] geo is a geo object, if it is inavlid the function returns "0,0" (coordinates of
     * the point somewhere in Atlantic Ocean)
     * @param[in] latFirst specity the order of lat/lon in return value
     * @return a string "$lon,$lat" if latFirst is false, otherwise "$lat,$lon"
     */
    TString LocationAsString(const TGeoPtr& geo, const bool latFirst);

    /** Returns location as a string of given geo object.
     * It doesn't check if the data exists.
     * @see LocationAsString()
     * @param[in] geo is a geo object, if it is inavlid the function returns "0,0"
     * @return a string "$lon,$lat"
     */
    TString LocationAsString(const TGeoPtr& geo);

    /** Check if object is in one of KUBR countries
     * @code
     * IsKUBR(geodb->Find(213)); // is true
     * @endcode
     * @return true if object is in KUBR
     */
    bool IsKUBR(const TGeoPtr& geo);
} // namespace NGeoDB
