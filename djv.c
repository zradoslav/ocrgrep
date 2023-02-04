#include <libdjvu/ddjvuapi.h>

#include "page.h"

static ddjvu_context_t* ctx;

void* djvu_create(const char* file)
{
	if(!ctx) {
		ctx = ddjvu_context_create("djv ctx");
		if(!ctx)
			goto error_ctx;
	}

	ddjvu_document_t* doc = ddjvu_document_create_by_filename(ctx, file, 0);
	if(!doc)
		goto error_doc;

	while(!ddjvu_document_decoding_done(doc))
		continue;

	return (void*)doc;

error_doc:
	ddjvu_context_release(ctx);
error_ctx:
	return NULL;
}

void djvu_release(void* doc)
{
	if(!ctx)
		return;

	ddjvu_document_t* _doc = (ddjvu_document_t*)doc;

	ddjvu_document_release(_doc);
	ddjvu_context_release(ctx);
}

int djvu_page_next(void* doc, Page* page)
{
	static int pageno = 0;
	ddjvu_document_t* _doc = (ddjvu_document_t*)doc;

	ddjvu_page_t* _page = ddjvu_page_create_by_pageno(_doc, pageno);
	if(!_page)
		goto error_page;

	while(!ddjvu_page_decoding_done(_page))
		continue;

	ddjvu_rect_t rect;
	rect.w = ddjvu_page_get_width(_page);
	rect.h = ddjvu_page_get_height(_page);

	unsigned int stride;
	ddjvu_format_t* format = NULL;
	ddjvu_page_type_t type = ddjvu_page_get_type(_page);
	if(type == DDJVU_PAGETYPE_BITONAL)
	{
		format = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, NULL);
		stride = rect.w;
	}
	else
	{
		unsigned int masks[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
		format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
		stride = rect.w * 4;
	}
	ddjvu_format_set_row_order(format, 1);

	unsigned char* buffer = malloc(stride * rect.h);
	if(!ddjvu_page_render(_page, DDJVU_RENDER_COLOR, &rect, &rect, format, stride, (char*)buffer))
	{
		free(buffer);
		goto error_rend;
	}

	page->data = buffer;
	page->width = rect.w;
	page->height = rect.h;
	page->bytes_per_pixel = (type == DDJVU_PAGETYPE_BITONAL) ? 1 : 3;
	page->bytes_per_line = stride;
	page->text = NULL;

	pageno++;

error_rend:
	ddjvu_format_release(format);
	ddjvu_page_release(_page);
	return 0;

error_page:
	return -1;
}
