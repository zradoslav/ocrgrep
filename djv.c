#include <libdjvu/ddjvuapi.h>

#include "page.h"

#define AS_IMPL(ptr) ((Impl*)(ptr))

typedef struct {
	ddjvu_context_t *ctx;
	ddjvu_document_t *doc;
	int pageno;
} Impl;

void *djvu_create(const char *file);
int   djvu_page_next(void *impl, Page *page);
void  djvu_release(void *impl);

void *
djvu_create(const char *file)
{
	Impl *impl;
	ddjvu_context_t *ctx;
	ddjvu_document_t* doc;

	if (!(ctx = ddjvu_context_create("djv ctx")))
		return NULL;

	if (!(doc = ddjvu_document_create_by_filename(ctx, file, 0))) {
		ddjvu_context_release(ctx);
		return NULL;
	}
	while (!ddjvu_document_decoding_done(doc))
		continue;

	impl = malloc(sizeof(Impl));
	impl->ctx = ctx;
	impl->doc = doc;
	impl->pageno = 0;

	return impl;
}

int
djvu_page_next(void *impl, Page *page)
{
	ddjvu_document_t *doc;
	ddjvu_page_t *p;
	ddjvu_rect_t r;
	unsigned int stride, mask[4];
	ddjvu_format_t *f;
	ddjvu_page_type_t t;
	char *data;

	if (!impl)
		return -1;

	doc = AS_IMPL(impl)->doc;

	p = ddjvu_page_create_by_pageno(doc, AS_IMPL(impl)->pageno);
	if (!p)
		return -1;

	while (!ddjvu_page_decoding_done(p))
		continue;

	r.w = ddjvu_page_get_width(p);
	r.h = ddjvu_page_get_height(p);

	t = ddjvu_page_get_type(p);
	if (t == DDJVU_PAGETYPE_BITONAL) {
		f = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, NULL);
		stride = r.w;
	} else {
		mask[0] = 0x00ff0000;
		mask[1] = 0x0000ff00;
		mask[2] = 0x000000ff;
		mask[3] = 0xff000000;
		f = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, mask);
		stride = r.w * 4;
	}
	ddjvu_format_set_row_order(f, 1);

	data = malloc(stride * r.h);
	if (!ddjvu_page_render(p, DDJVU_RENDER_COLOR, &r, &r, f, stride, data)) {
		free(data);
		ddjvu_format_release(f);
		ddjvu_page_release(p);
		return -1;
	}

	page->data = (unsigned char*)data;
	page->width = r.w;
	page->height = r.h;
	page->bytes_per_pixel = (t == DDJVU_PAGETYPE_BITONAL) ? 1 : 3;
	page->bytes_per_line = stride;
	page->text = NULL;

	AS_IMPL(impl)->pageno++;

	ddjvu_format_release(f);
	ddjvu_page_release(p);
	return 0;
}

void
djvu_release(void *impl)
{
	if (!impl)
		return;

	ddjvu_document_release(AS_IMPL(impl)->doc);
	ddjvu_context_release(AS_IMPL(impl)->ctx);

	free(impl);
}
