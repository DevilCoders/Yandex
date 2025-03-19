package marketplace

type productManager interface {
	GetProductByID(params GetProductParams) (*Product, error)
	GetVersionByID(params GetVersionParams) (*Version, error)
	SyncProduct(params SyncProductParams) (*Operation, error)
	SyncVersion(params SyncVersionParams) (*Operation, error)
}

type Product struct {
	ID             string           `json:"id,omitempty"`
	Name           string           `json:"name"`
	State          string           `json:"state,omitempty"`
	TermsOfService []TermsOfService `json:"termsOfService"`
	VersionID      string           `json:"productId"`
	TariffID       string           `json:"tariffId"`
	MarketingInfo  MarketingInfo    `json:"marketingInfo"`
	Source         Source           `json:"source"`
	Payload        Payload          `json:"payload"`
	PublisherID    string           `json:"publisherId"`
	Type           string           `json:"type"`
}

type Version struct {
	ID             string           `json:"id,omitempty"`
	State          string           `json:"state,omitempty"`
	TermsOfService []TermsOfService `json:"termsOfService"`
	ProductID      string           `json:"productId"`
	TariffID       string           `json:"tariffId"`
	MarketingInfo  MarketingInfo    `json:"marketingInfo"`
	Source         Source           `json:"source"`
	Payload        Payload          `json:"payload"`
}

type TermsOfService struct {
	Title LocalizedString `json:"title"`
	Type  string          `json:"type"`
	URL   string          `json:"url"`
}

type MarketingInfo struct {
	UseCases         LocalizedString `json:"useCases"`
	LogoSource       LogoSource      `json:"logoSource"`
	Name             LocalizedString `json:"name"`
	Description      LocalizedString `json:"description"`
	Support          LocalizedString `json:"support"`
	ShortDescription LocalizedString `json:"shortDescription"`
	Links            LocalizedLinks  `json:"links"`
	Tutorial         LocalizedString `json:"tutorial"`
	PriceDisplayType string          `json:"price_display_type,omitempty"`
}

type StorageObject struct {
	BucketName string `json:"bucket_name"`
	ObjectName string `json:"object_name"`
}

type LogoSource struct {
	StorageObject *StorageObject `json:"storage_object,omitempty"`
	VersionID     *string        `json:"versionId,omitempty"`
	Content       *string        `json:"content,omitempty"`
}

type LocalizedLinks struct {
	En Links `json:"en"`
	Ru Links `json:"ru"`
}

type Links struct {
	Links []Link `json:"links"`
}

type Link struct {
	Title string `json:"title"`
	URL   string `json:"url"`
}

type Source struct {
	ComputeImage ComputeImageSource `json:"computeImage"`
}

type ComputeImageSource struct {
	ImageID  string `json:"imageId"`
	FolderID string `json:"folderId,omitempty"`
}

type Payload struct {
	ComputeImage ComputeImagePayload `json:"computeImage"`
}

type ComputeImagePayload struct {
	ImageID      string       `json:"imageId,omitempty"`
	PackageInfo  PackageInfo  `json:"packageInfo"`
	ResourceSpec ResourceSpec `json:"resourceSpec"`
	FormID       string       `json:"formId"`
}

type PackageInfo struct {
	OS              Os                `json:"os"`
	PackageContents []PackageContents `json:"packageContents"`
}

type Os struct {
	Version string `json:"version"`
	Family  string `json:"family"`
	Name    string `json:"name"`
}

type PackageContents struct {
	Version string `json:"version"`
	Name    string `json:"name"`
}

type ResourceSpec struct {
	NetworkInterfaces *BoundedValue `json:"networkInterfaces,omitempty"`
	Memory            BoundedValue  `json:"memory"`
	CPU               BoundedValue  `json:"cpu"`
	DiskSize          BoundedValue  `json:"disk_size"`
	ComputePlatforms  []string      `json:"computePlatforms,omitempty"`
	GPU               *BoundedValue `json:"gpu,omitempty"`
	CPUFraction       BoundedValue  `json:"cpuFraction"`
}

type BoundedValue struct {
	Min         *int `json:"min,omitempty"`
	Recommended *int `json:"recommended,omitempty"`
	Max         *int `json:"max,omitempty"`
}

type LocalizedString struct {
	En string `json:"en"`
	Ru string `json:"ru"`
}

type SyncProductParams struct {
	//    maybe should add omitempty
	ID          string `json:"id"`
	PublisherID string `json:"publisherId"`
	Type        string `json:"type"`
	Name        string `json:"name"`
}

type SyncVersionParams struct {
	//    related_products = ListType(ModelType(ReferenceRequest))

	//    restrictions = ModelType(RestrictionsRequest) - not required
	CategoryIDs    []string         `json:"category_ids"`
	ID             string           `json:"id"`
	ProductID      string           `json:"productId"`
	PublisherID    string           `json:"publisherId"`
	TariffID       string           `json:"tariffId"`
	MarketingInfo  MarketingInfo    `json:"marketingInfo"`
	TermsOfService []TermsOfService `json:"termsOfService"`
	Payload        Payload          `json:"payload"`
	Source         Source           `json:"source"`
	Tags           []string         `json:"tags"`
	VendorID       string           `json:"vendorId,omitempty"`
}

type GetProductParams struct {
	ProductID   string `json:"ProductId"`
	PublisherID string `json:"PublisherId"`
}

type GetVersionParams struct {
	ProductID   string `json:"ProductId"`
	PublisherID string `json:"PublisherId"`
	VersionID   string `json:"VersionId"`
}

func (s *Session) GetProductByID(params GetProductParams) (*Product, error) {
	var result Product

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetPathParam("publisherID", params.PublisherID).
		SetPathParam("productID", params.PublisherID).
		Get("/marketplace/v2/private/publishers/{publisherID}/products/{productID}")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}

func (s *Session) GetVersionByID(params GetVersionParams) (*Version, error) {
	var result Version

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetPathParam("publisherID", params.PublisherID).
		SetPathParam("productID", params.ProductID).
		SetPathParam("versionID", params.VersionID).
		Get("/marketplace/v2/private/publishers/{publisherID}/products/{productID}/versions/{versionID}")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}

func (s *Session) SyncProduct(params SyncProductParams) (*Operation, error) {
	var result Operation

	response, err := s.ctxAuthRequest().
		SetBody(params).
		SetResult(&result).
		SetPathParam("publisherID", params.PublisherID).
		Put("/marketplace/v2/private/publishers/{publisherID}/products")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}

func (s *Session) SyncVersion(params SyncVersionParams) (*Operation, error) {
	var result Operation

	response, err := s.ctxAuthRequest().
		SetBody(params).
		SetResult(&result).
		SetPathParam("publisherID", params.PublisherID).
		SetPathParam("productID", params.ProductID).
		Put("/marketplace/v2/private/publishers/{publisherID}/products/{productID}/versions")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}
