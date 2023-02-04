#include <string.h>

#include <mupdf/fitz.h>

#include "page.h"

#define AS_IMPL(ptr) ((Impl*)(ptr))

typedef struct {
	fz_context *ctx;
	fz_document *doc;
	int pageno;
} Impl;

void *pdf_create(const char *file);
int   pdf_page_next(void *impl, Page *page);
void  pdf_release(void *impl);

void *
pdf_create(const char* file)
{
	Impl *impl;
	fz_context *ctx;
	fz_document *doc;

	if (!(ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED)))
		return NULL;
	fz_register_document_handlers(ctx);

	if (!(doc = fz_open_document(ctx, file))) {
		fz_drop_context(ctx);
		return NULL;
	}

	impl = malloc(sizeof(Impl));
	impl->ctx = ctx;
	impl->doc = doc;
	impl->pageno = 0;

	return impl;
}

int
pdf_page_next(void *impl, Page *page)
{
	fz_context *ctx;
	fz_matrix ctm;
	fz_page *p;
	fz_pixmap *pix;
	size_t nbytes;

	fz_stext_page *stext_p;
	fz_device *stext_d;
	fz_irect bbox;
	fz_point corner_tl, corner_br;

	ctx = AS_IMPL(impl)->ctx;

	if (!(p = fz_load_page(ctx, AS_IMPL(impl)->doc, AS_IMPL(impl)->pageno)))
		return -1;

	ctm = fz_scale(1, 1);
	/* 'fz_device_gray' can be used to reduce mem by 3 */
	if (!(pix = fz_new_pixmap_from_page(ctx, p, ctm, fz_device_rgb(ctx), 0))) {
		fz_drop_page(ctx, p);
		return -1;
	}

	page->width = fz_pixmap_width(ctx, pix);
	page->height = fz_pixmap_height(ctx, pix);
	page->bytes_per_pixel = pix->n;
	page->bytes_per_line = fz_pixmap_stride(ctx, pix);

	nbytes = page->height * page->bytes_per_line;
	page->data = malloc(nbytes);
	memcpy(page->data, fz_pixmap_samples(ctx, pix), nbytes);

	if ((stext_p = fz_new_stext_page_from_page(ctx, p, NULL))) {
		if ((stext_d = fz_new_stext_device(ctx, stext_p, NULL))) {
			fz_run_page(ctx, p, stext_d, ctm, NULL);

			bbox = fz_pixmap_bbox(ctx, pix);
			corner_tl = fz_make_point(bbox.x0, bbox.y0);
			corner_br = fz_make_point(bbox.x1, bbox.y1);
#ifdef _WIN32
			page->text = fz_copy_selection(ctx, stext_p, corner_tl, corner_br, 1);
#else
			page->text = fz_copy_selection(ctx, stext_p, corner_tl, corner_br, 0);
#endif
			fz_close_device(ctx, stext_d);
			fz_drop_device(ctx, stext_d);
		}
		fz_drop_stext_page(ctx, stext_p);
	}
	AS_IMPL(impl)->pageno++;

	fz_drop_pixmap(ctx, pix);
	fz_drop_page(ctx, p);
	return 0;
}

void
pdf_release(void *impl)
{
	if (!impl)
		return;

	fz_drop_document(AS_IMPL(impl)->ctx, AS_IMPL(impl)->doc);
	fz_drop_context(AS_IMPL(impl)->ctx);

	free(impl);
}
