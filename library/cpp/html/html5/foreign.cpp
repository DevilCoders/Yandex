#include "foreign.h"

#include <string.h>

#include <contrib/libs/libc_compat/string.h>

namespace NHtml5 {
#define STATIC_STRING(literal) \
    { literal, sizeof(literal) - 1 }
#define TERMINATOR \
    { "", 0 }

    struct TReplacementEntry {
        const TStringPiece From;
        const TStringPiece To;
    };

#define REPLACEMENT_ENTRY(from, to) \
    { STATIC_STRING(from), STATIC_STRING(to) }

    // Static data for SVG attribute replacements.
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#adjust-svg-attributes
    /*
static const TReplacementEntry kSvgAttributeReplacements[] = {
    REPLACEMENT_ENTRY("attributename", "attributeName"),
    REPLACEMENT_ENTRY("attributetype", "attributeType"),
    REPLACEMENT_ENTRY("basefrequency", "baseFrequency"),
    REPLACEMENT_ENTRY("baseprofile", "baseProfile"),
    REPLACEMENT_ENTRY("calcmode", "calcMode"),
    REPLACEMENT_ENTRY("clippathunits", "clipPathUnits"),
    REPLACEMENT_ENTRY("contentscripttype", "contentScriptType"),
    REPLACEMENT_ENTRY("contentstyletype", "contentStyleType"),
    REPLACEMENT_ENTRY("diffuseconstant", "diffuseConstant"),
    REPLACEMENT_ENTRY("edgemode", "edgeMode"),
    REPLACEMENT_ENTRY("externalresourcesrequired", "externalResourcesRequired"),
    REPLACEMENT_ENTRY("filterres", "filterRes"),
    REPLACEMENT_ENTRY("filterunits", "filterUnits"),
    REPLACEMENT_ENTRY("glyphref", "glyphRef"),
    REPLACEMENT_ENTRY("gradienttransform", "gradientTransform"),
    REPLACEMENT_ENTRY("gradientunits", "gradientUnits"),
    REPLACEMENT_ENTRY("kernelmatrix", "kernelMatrix"),
    REPLACEMENT_ENTRY("kernelunitlength", "kernelUnitLength"),
    REPLACEMENT_ENTRY("keypoints", "keyPoints"),
    REPLACEMENT_ENTRY("keysplines", "keySplines"),
    REPLACEMENT_ENTRY("keytimes", "keyTimes"),
    REPLACEMENT_ENTRY("lengthadjust", "lengthAdjust"),
    REPLACEMENT_ENTRY("limitingconeangle", "limitingConeAngle"),
    REPLACEMENT_ENTRY("markerheight", "markerHeight"),
    REPLACEMENT_ENTRY("markerunits", "markerUnits"),
    REPLACEMENT_ENTRY("markerwidth", "markerWidth"),
    REPLACEMENT_ENTRY("maskcontentunits", "maskContentUnits"),
    REPLACEMENT_ENTRY("maskunits", "maskUnits"),
    REPLACEMENT_ENTRY("numoctaves", "numOctaves"),
    REPLACEMENT_ENTRY("pathlength", "pathLength"),
    REPLACEMENT_ENTRY("patterncontentunits", "patternContentUnits"),
    REPLACEMENT_ENTRY("patterntransform", "patternTransform"),
    REPLACEMENT_ENTRY("patternunits", "patternUnits"),
    REPLACEMENT_ENTRY("pointsatx", "pointsAtX"),
    REPLACEMENT_ENTRY("pointsaty", "pointsAtY"),
    REPLACEMENT_ENTRY("pointsatz", "pointsAtZ"),
    REPLACEMENT_ENTRY("preservealpha", "preserveAlpha"),
    REPLACEMENT_ENTRY("preserveaspectratio", "preserveAspectRatio"),
    REPLACEMENT_ENTRY("primitiveunits", "primitiveUnits"),
    REPLACEMENT_ENTRY("refx", "refX"),
    REPLACEMENT_ENTRY("refy", "refY"),
    REPLACEMENT_ENTRY("repeatcount", "repeatCount"),
    REPLACEMENT_ENTRY("repeatdur", "repeatDur"),
    REPLACEMENT_ENTRY("requiredextensions", "requiredExtensions"),
    REPLACEMENT_ENTRY("requiredfeatures", "requiredFeatures"),
    REPLACEMENT_ENTRY("specularconstant", "specularConstant"),
    REPLACEMENT_ENTRY("specularexponent", "specularExponent"),
    REPLACEMENT_ENTRY("spreadmethod", "spreadMethod"),
    REPLACEMENT_ENTRY("startoffset", "startOffset"),
    REPLACEMENT_ENTRY("stddeviation", "stdDeviation"),
    REPLACEMENT_ENTRY("stitchtiles", "stitchTiles"),
    REPLACEMENT_ENTRY("surfacescale", "surfaceScale"),
    REPLACEMENT_ENTRY("systemlanguage", "systemLanguage"),
    REPLACEMENT_ENTRY("tablevalues", "tableValues"),
    REPLACEMENT_ENTRY("targetx", "targetX"),
    REPLACEMENT_ENTRY("targety", "targetY"),
    REPLACEMENT_ENTRY("textlength", "textLength"),
    REPLACEMENT_ENTRY("viewbox", "viewBox"),
    REPLACEMENT_ENTRY("viewtarget", "viewTarget"),
    REPLACEMENT_ENTRY("xchannelselector", "xChannelSelector"),
    REPLACEMENT_ENTRY("ychannelselector", "yChannelSelector"),
    REPLACEMENT_ENTRY("zoomandpan", "zoomAndPan"),
};
*/

    static const TReplacementEntry kSvgTagReplacements[] = {
        REPLACEMENT_ENTRY("altglyph", "altGlyph"),
        REPLACEMENT_ENTRY("altglyphdef", "altGlyphDef"),
        REPLACEMENT_ENTRY("altglyphitem", "altGlyphItem"),
        REPLACEMENT_ENTRY("animatecolor", "animateColor"),
        REPLACEMENT_ENTRY("animatemotion", "animateMotion"),
        REPLACEMENT_ENTRY("animatetransform", "animateTransform"),
        REPLACEMENT_ENTRY("clippath", "clipPath"),
        REPLACEMENT_ENTRY("feblend", "feBlend"),
        REPLACEMENT_ENTRY("fecolormatrix", "feColorMatrix"),
        REPLACEMENT_ENTRY("fecomponenttransfer", "feComponentTransfer"),
        REPLACEMENT_ENTRY("fecomposite", "feComposite"),
        REPLACEMENT_ENTRY("feconvolvematrix", "feConvolveMatrix"),
        REPLACEMENT_ENTRY("fediffuselighting", "feDiffuseLighting"),
        REPLACEMENT_ENTRY("fedisplacementmap", "feDisplacementMap"),
        REPLACEMENT_ENTRY("fedistantlight", "feDistantLight"),
        REPLACEMENT_ENTRY("feflood", "feFlood"),
        REPLACEMENT_ENTRY("fefunca", "feFuncA"),
        REPLACEMENT_ENTRY("fefuncb", "feFuncB"),
        REPLACEMENT_ENTRY("fefuncg", "feFuncG"),
        REPLACEMENT_ENTRY("fefuncr", "feFuncR"),
        REPLACEMENT_ENTRY("fegaussianblur", "feGaussianBlur"),
        REPLACEMENT_ENTRY("feimage", "feImage"),
        REPLACEMENT_ENTRY("femerge", "feMerge"),
        REPLACEMENT_ENTRY("femergenode", "feMergeNode"),
        REPLACEMENT_ENTRY("femorphology", "feMorphology"),
        REPLACEMENT_ENTRY("feoffset", "feOffset"),
        REPLACEMENT_ENTRY("fepointlight", "fePointLight"),
        REPLACEMENT_ENTRY("fespecularlighting", "feSpecularLighting"),
        REPLACEMENT_ENTRY("fespotlight", "feSpotLight"),
        REPLACEMENT_ENTRY("fetile", "feTile"),
        REPLACEMENT_ENTRY("feturbulence", "feTurbulence"),
        REPLACEMENT_ENTRY("foreignobject", "foreignObject"),
        REPLACEMENT_ENTRY("glyphref", "glyphRef"),
        REPLACEMENT_ENTRY("lineargradient", "linearGradient"),
        REPLACEMENT_ENTRY("radialgradient", "radialGradient"),
        REPLACEMENT_ENTRY("textpath", "textPath"),
    };

#undef TERMINATOR
#undef STATIC_STRING

    const char* NormalizeSvgTagname(const TStringPiece& tag) {
        for (const auto& entry : kSvgTagReplacements) {
            if (tag.Length != entry.From.Length) {
                continue;
            }
            if (strnicmp(tag.Data, entry.From.Data, tag.Length) == 0) {
                return entry.To.Data;
            }
        }
        return nullptr;
    }
}
