namespace NCbir;

struct TImageDownloaderParametersScheme {
    AllowRawImage : bool (default = true);
    
    AllowAvatarsInternalImage : bool (default = true);

    AllowExternalImageOverAvatars : bool (default = true);
    AvatarsNamespace : string (default = "images-cbir");
    AvatarsPrefix : string (default = "");
    AvatarsSize : string (default = "ocr");

    AllowIm0tubImage : bool (default = true);

    ReuseOldData : bool (default = false);
    ReusePrefix : string (default = "");
};
