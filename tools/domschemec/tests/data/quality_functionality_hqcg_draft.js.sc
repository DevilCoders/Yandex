namespace NHQCG::NDraftJs;

struct TBlock {
    key: string (required);
    text: string (required);
    type: string (required);
    depth: ui32;

    struct TInlinedStyleRange {
        offset: ui32 (required);
        length: ui32 (required);
        style: string (required); //"BOLD"
    };
    inlineStyleRanges: [TInlinedStyleRange];

    struct TEntityRange {
        offset: ui32 (required);
        length: ui32 (required);
        key: ui32 (required);
    };
    entityRanges: [TEntityRange];

    data: any;
};

struct TEntityMapNode {
    type: string (required);
    mutability: string;
    data: any;
};

struct TObject {
    blocks    : [TBlock] (required);
    entityMap : {string -> TEntityMapNode} (required);
};
