package marketplace

type APISession interface {
	categoryManager
	operationManager
	publisherManager
	productManager
	tariffManager
}
