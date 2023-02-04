#include <strings.h>

typedef struct {
	const char *extension;
	DocHandle methods;
} DocHandleMap;

static const char* get_extension(const char* fpath);
static DocHandle* get_handle(const char *ext);

/* pdf */
extern void* pdf_create(const char* fpath);
extern void pdf_release(void* ctx);
extern int pdf_page_next(void* ctx, Page* page);

/* djvu */
extern void* djvu_create(const char* fpath);
extern void djvu_release(void* ctx);
extern int djvu_page_next(void* ctx, Page* page);

static DocHandleMap dochandlers[] = {
	{ ".djv", { &djvu_create, &djvu_release, djvu_page_next, NULL } },
	{ ".djvu", { &djvu_create, &djvu_release, djvu_page_next, NULL } },
	{ ".pdf", { &pdf_create, &pdf_release, pdf_page_next, NULL } }
};

const char*
get_extension(const char* fpath)
{
	const char* dot = strrchr(fpath, '.');
	if (!dot || dot == fpath)
		return NULL;
	return dot;
}

DocHandle*
get_handle(const char *fpath) {
	const char* ext = get_extension(fpath);

	for (int i = 0; i < sizeof(dochandlers) / sizeof(*dochandlers); ++i)
		if (strcasecmp(ext, dochandlers[i].extension) == 0)
			return &dochandlers[i].methods;

	return NULL;
}