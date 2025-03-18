cdef extern from "./cJSON.h":
	ctypedef struct cJSON:
		cJSON *next
		cJSON *prev
		cJSON *child

		int type

		char *valuestring
		int valueint
		double valuedouble
		char *string


	ctypedef struct cJSON_Hooks:
		void *(*malloc_fn)(size_t sz)
		void (*free_fn)(void *ptr)

	extern void cJSON_InitHooks(cJSON_Hooks *hooks)

	extern cJSON *cJSON_Parse(const char *value)
	extern char *cJSON_Print(cJSON *item)
	extern char *cJSON_PrintUnformatted(cJSON *item)
	extern char *cJSON_PrintBuffered(cJSON *item, int prebuffer, int fmt)
	extern void cJSON_Delete(cJSON *c)
	extern int cJSON_GetArraySize(cJSON *array)
	extern cJSON *cJSON_GetArrayItem(cJSON *array, int item)
	extern cJSON *cJSON_GetObjectItem(cJSON *object, const char *string)
	
	
	extern cJSON *cJSON_GetObjectItem(cJSON *object,const char *string)
	extern int cJSON_HasObjectItem(cJSON *object,const char *string)
	extern const char *cJSON_GetErrorPtr()
	extern cJSON *cJSON_CreateNull()
	extern cJSON *cJSON_CreateTrue()
	extern cJSON *cJSON_CreateFalse()
	extern cJSON *cJSON_CreateBool(int b)
	extern cJSON *cJSON_CreateNumber(double num)
	extern cJSON *cJSON_CreateString(const char *string)
	extern cJSON *cJSON_CreateArray()
	extern cJSON *cJSON_CreateObject()


	extern cJSON *cJSON_CreateIntArray(const int *numbers,int count)
	extern cJSON *cJSON_CreateFloatArray(const float *numbers,int count)
	extern cJSON *cJSON_CreateDoubleArray(const double *numbers,int count)
	extern cJSON *cJSON_CreateStringArray(const char **strings,int count)


	extern void cJSON_AddItemToArray(cJSON *array, cJSON *item)
	extern void	cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)
	extern void	cJSON_AddItemToObjectCS(cJSON *object,const char *string,cJSON *item)
	extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)
	extern void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item)


	extern cJSON *cJSON_DetachItemFromArray(cJSON *array,int which)
	extern void   cJSON_DeleteItemFromArray(cJSON *array,int which)
	extern cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string)
	extern void   cJSON_DeleteItemFromObject(cJSON *object,const char *string)


	extern void cJSON_InsertItemInArray(cJSON *array,int which,cJSON *newitem)
	extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem)
	extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem)


	extern cJSON *cJSON_Duplicate(cJSON *item,int recurse)


	extern cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated)

	extern void cJSON_Minify(char *json)
