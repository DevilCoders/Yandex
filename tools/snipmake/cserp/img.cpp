#include "img.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <util/generic/noncopyable.h>
#include <util/digest/numeric.h>

#include <contrib/libs/ImageMagick/magick/api.h>

namespace NIMagic {
    class TException : private TNonCopyable {
    private:
        ExceptionInfo Info;

    private:
        void Clear() {
            DestroyExceptionInfo(&Info);
            if (Info.exceptions != nullptr) {
                Info.exceptions = (void *) DestroyLinkedList((LinkedListInfo *)Info.exceptions, (void *(*)(void *)) nullptr);
            }
            if (Info.semaphore != nullptr) {
                DestroySemaphoreInfo(&Info.semaphore);
            }
        }

    public:
        TException() {
            GetExceptionInfo(&Info);
        }
        ~TException() {
            Clear();
        }

        void Reset() {
            Clear();
            GetExceptionInfo(&Info);
        }
        ExceptionInfo* Get() {
            return &Info;
        }
    };
    class TImageInfo : private TNonCopyable {
    private:
        ImageInfo* Info;

    public:
        TImageInfo()
          : Info(CloneImageInfo(nullptr))
        {
        }
        ~TImageInfo()
        {
            DestroyImageInfo(Info);
        }

        ImageInfo* Get() {
            return Info;
        }
    };
    class TView : private TNonCopyable {
    private:
        CacheView* View;

    public:
        explicit TView(const Image* image)
          : View(AcquireCacheView(image))
        {
        }
        ~TView() {
            if (View) {
                DestroyCacheView(View);
            }
        }

        CacheView* Get() {
            return View;
        }
    };
    typedef TSimpleSharedPtr<TView> TViewPtr;
    class TPixels {
    private:
        TViewPtr View;
        const PixelPacket* Data;
        size_t W;
        size_t H;

    public:
        explicit TPixels(const TViewPtr& view, const PixelPacket* data, size_t w, size_t h)
          : View(view)
          , Data(data)
          , W(w)
          , H(h)
        {
        }

        size_t GetCount() const {
            return W * H;
        }
        const PixelPacket* Get() const {
            return Data;
        }
        const PixelPacket& Get(size_t x, size_t y) const {
            return Data[x + y * W];
        }
        ui64 GetHash() const {
            ui64 res = 0;
            res = CombineHashes<ui64>(res, W);
            res = CombineHashes<ui64>(res, H);
            for (size_t i = 0; i < W * H; ++i) {
                res = CombineHashes<ui64>(res, Data[i].red);
                res = CombineHashes<ui64>(res, Data[i].green);
                res = CombineHashes<ui64>(res, Data[i].blue);
            }
            return res;
        }
    };
    class TBlobImage : private TNonCopyable {
    private:
        TException Except;
        TImageInfo Info;
        Image* Img;

    public:
        explicit TBlobImage(const TBlob& blob)
          : Except()
          , Info()
          , Img(BlobToImage(Info.Get(), blob.Data(), blob.Size(), Except.Get()))
        {
            if (Img) {
                SetImageColorspace(Img, RGBColorspace);
                SetImageStorageClass(Img, DirectClass);
                if (Img->matte == MagickFalse) {
                    SetImageAlphaChannel(Img, OpaqueAlphaChannel);
                }
            }
        }
        ~TBlobImage() {
            if (Img) {
                DestroyImageList(Img);
            }
        }

        bool Valid() const {
            return !!Img;
        }
        size_t GetW() const {
            return Img->columns;
        }
        size_t GetH() const {
            return Img->rows;
        }
        TPixels GetPixels(size_t x, size_t y, size_t w, size_t h) const {
            TException exc;
            TViewPtr view(new TView(Img));
            return TPixels(view, GetCacheViewVirtualPixels(view->Get(), x, y, w, h, exc.Get()), w, h);
        }
        TPixels GetAll() const {
            return GetPixels(0, 0, GetW(), GetH());
        }
    };
}

namespace NSnippets {
    struct TCanvas::TImpl {
        NIMagic::TBlobImage Data;
        TImpl(const TBlob& rawPng)
          : Data(rawPng)
        {
        }
    };

    TCanvas::TCanvas(const TBlob& rawPng)
      : Impl(new TImpl(rawPng))
    {
    }
    TCanvas::~TCanvas() {
    }

    TCanvasPtr TCanvas::FromBlob(const TBlob& rawPng) {
        TCanvasPtr res(new TCanvas(rawPng));
        if (!res->Impl->Data.Valid()) {
            return TCanvasPtr();
        }
        return res;
    }
    TCanvasPtr TCanvas::FromBase64(const TString& pngBase64) {
        return FromBlob(TBlob::FromString(Base64Decode(pngBase64)));
    }

    size_t TCanvas::GetW() const {
        return Impl->Data.GetW();
    }
    size_t TCanvas::GetH() const {
        return Impl->Data.GetH();
    }
    ui64 TCanvas::GetHash() const {
        return Impl->Data.GetAll().GetHash();
    }
    ui64 TCanvas::GetSubHash(size_t x, size_t y, size_t w, size_t h) {
        return Impl->Data.GetPixels(x, y, w, h).GetHash();
    }
}
