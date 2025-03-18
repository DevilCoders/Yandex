namespace NInterleaving;


struct TGInfoDumpConfig {
    GroupID : string;
};

struct TCutSnippetsConfig {
    Enabled : bool (default = false);
    CutTitle : bool (default = false);
    CutBody : bool (default = false);

    TitleRows : double (default = 1);
    TitleFontSize : double (default = 1);
    TitlePixelsInLine : i32 (default = 1);
    BodyRows : double (default = 1);
    RowLen : i32 (default = 200);
    FontSize : double (default = 14);
};
