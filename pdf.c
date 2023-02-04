#include <stdlib.h>
#include <string.h>

#include <mupdf/fitz.h>

#include "page.h"

static fz_context* ctx;

void* pdf_create(const char* file)
{
	ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if(!ctx)
		goto error_ctx;

	fz_register_document_handlers(ctx);
	fz_document* doc = fz_open_document(ctx, file);
	if(!doc)
		goto error_doc;

	return (void*)doc;

error_doc:
	fz_drop_context(ctx);
error_ctx:
	return NULL;
}

void pdf_release(void* doc)
{
	if(!ctx)
		return;

	fz_document* _doc = (fz_document*)doc;

	fz_drop_document(ctx, _doc);
	fz_drop_context(ctx);
}

int pdf_page_next(void* doc, Page* page)
{
	int ret = 0;

	static int pageno = 0;
	fz_document* _doc = (fz_document*)doc;

	fz_matrix ctm = fz_scale(1, 1);

	fz_page* _page = fz_load_page(ctx, _doc, pageno);
	if(!_page)
	{
		ret = -1;
		goto error_page;
	}

	/* 'fz_device_gray' can be used to reduce mem by 3 */
	fz_pixmap* pix = fz_new_pixmap_from_page(ctx, _page, ctm, fz_device_rgb(ctx), 0);
	if(!pix)
	{
		ret = -1;
		goto error_pix;
	}

	page->width = fz_pixmap_width(ctx, pix);
	page->height = fz_pixmap_height(ctx, pix);
	page->bytes_per_pixel = pix->n;
	page->bytes_per_line = fz_pixmap_stride(ctx, pix);

	size_t len = page->height * page->bytes_per_line;
	page->data = malloc(len);
	memcpy(page->data, fz_pixmap_samples(ctx, pix), len);

	fz_stext_page* stext_page = fz_new_stext_page_from_page(ctx, _page, NULL);
	if(stext_page)
	{
		fz_device* stext_device = fz_new_stext_device(ctx, stext_page, NULL);
		if(stext_device)
		{
			fz_run_page(ctx, _page, stext_device, ctm, NULL);

			fz_irect bound_box = fz_pixmap_bbox(ctx, pix);
			fz_point corner_tl = {bound_box.x0, bound_box.y0};
			fz_point corner_br = {bound_box.x1, bound_box.y1};
#ifdef _WIN32
			page->text = fz_copy_selection(ctx, stext_page, corner_tl, corner_br, 1);
#else
			page->text = fz_copy_selection(ctx, stext_page, corner_tl, corner_br, 0);
#endif
			fz_close_device(ctx, stext_device);
			fz_drop_device(ctx, stext_device);
		}
		fz_drop_stext_page(ctx, stext_page);
	}
	pageno++;

	fz_drop_pixmap(ctx, pix);
error_pix:
	fz_drop_page(ctx, _page);
error_page:
	return ret;
}
