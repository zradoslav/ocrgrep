#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <unistd.h>

#include <tesseract/capi.h>

#include "page.h"

typedef struct {
	void *(*create)(const char*);
	void (*release)(void*);
	int (*page_next)(void*, Page*);
	void *impl;
} DocHandle;

static void page_reset(Page *page);

/* ocrgrep */
static void die(const char *errstr, ...);
static void usage(void);
static char *ocr(TessBaseAPI *tapi, const Page *page);
static void match(const char *string, const regex_t *pregex, char **matches, int nmatches);

#define FIXME_SIZE 4

/* handler code goes here */
#include "handle.h"

void
page_reset(Page *page)
{
	if (page->data)
		free(page->data);
	if (page->text)
		free(page->text);
	memset(page, 0, sizeof(Page));
}

void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

void
usage()
{
	die("usage: ocrgrep [-aEinr] [-p pages] [-x language] [-X LIST] PATTERN FILE...\n");
}

char *
ocr(TessBaseAPI *tapi, const Page *page)
{
	char *text, *bytes;

	TessBaseAPISetImage(tapi, page->data, page->width, page->height, page->bytes_per_pixel, page->bytes_per_line);
	if (TessBaseAPIRecognize(tapi, NULL))
		return NULL;

	if (!(text = TessBaseAPIGetUTF8Text(tapi)))
		return NULL;

	bytes = strdup(text);
	TessDeleteText(text);

	return bytes;
}

void
match(const char *string, const regex_t *pregex, char **matches, int nmatches)
{
	const char *cursor = string;

	int ngroups = pregex->re_nsub + 1;
	regmatch_t *groups = malloc(ngroups * sizeof(regmatch_t));

	int match_num = 0;
	while (true)
	{
		if (regexec(pregex, cursor, ngroups, groups, 0) || match_num == nmatches)
			break;

		int offset = 0;
		for (int i = 0; i < ngroups; i++)
		{
			/* empty group, just skip */
			if (groups[i].rm_so == -1)
				continue;

			/* group 0 indicates whole match */
			if (i == 0)
			{
				offset = groups[i].rm_eo;
				continue;
			}

			/* strip some bytes here -- we can store only *start and length */
			int match_len = groups[i].rm_eo - groups[i].rm_so;
			matches[match_num] = strndup(cursor + groups[i].rm_so, match_len);
		}
		cursor += offset;
		match_num++;
	}

	free(groups);
}

int
main(int argc, char *argv[])
{
	int errcode;
	char *errbuf;
	size_t errbuf_size;
	regex_t pregex;

	char *file, *pattern;
	int opt_extended = 0, opt_icase = 0;
	int opt_number = 0, opt_recur = 0;
	int opt_builtin = 0;
	int opt_pages = 0;
	char *opt_tesslang = "eng", *opt_tessvars = NULL;

	TessBaseAPI* ocr_api;
	DocHandle* doc;
	Page page;
	int pageno = 0;

	int opt;
	while ((opt = getopt(argc, argv, "aEinrvp:x:X:")) != -1) {
		switch (opt) {
		case 'a':
			opt_builtin = 1;
			break;
		case 'E':
			opt_extended = REG_EXTENDED;
			break;
		case 'i':
			opt_icase = REG_ICASE;
			break;
		case 'n':
			opt_number = 1;
			break;
		case 'p':
			opt_pages = atoi(optarg);
			break;
		case 'r':
			opt_recur = 1;
			break;
		case 'x':
			opt_tesslang = optarg;
			if (*opt_tesslang == '-') usage();
			break;
		case 'X':
			opt_tessvars = optarg;
			if (*opt_tessvars == '-') usage();
			break;
		case 'v':
			die("ocrgrep " VERSION " \n");
		default:
			usage();
		}
	}

	if (opt_pages && opt_recur)
		die("Cannot set both page limit and whole scan\n");
	else if (!opt_pages && !opt_recur)
		die("Specify either -r (whole scan) or page limit (-p)\n");

	/* eat rest */
	if (argc - optind == 2) {
		pattern = argv[optind++];
		file = argv[optind++];
	} else {
		die("Invalid arguments\n");
	}

	if (!(doc = get_handle(file)))
		die("Unsupported file type\n");

	/* initialize OCR */
	ocr_api = TessBaseAPICreate();
	if (!ocr_api || TessBaseAPIInit3(ocr_api, NULL, opt_tesslang))
		die("OCR engine init failure\n");
	/* make Tesseract quiet */
	TessBaseAPISetVariable(ocr_api, "debug_file", "/dev/null");
	if (opt_tessvars) {
		char *var = strtok(opt_tessvars, ",="), *val;
		while (var) {
			if ((val = strtok(NULL, ",="))) {
				TessBaseAPISetVariable(ocr_api, var, val);
				var = strtok(NULL, ",=");
			} else {
				die("Invalid arguments\n");
			}
		}
	}

	/* create regexp */
	if ((errcode = regcomp(&pregex, pattern, opt_extended | opt_icase | REG_NEWLINE))) {
		errbuf_size = regerror(errcode, &pregex, NULL, 0);
		errbuf = malloc(errbuf_size);
		regerror(errcode, &pregex, errbuf, errbuf_size);
		die("regcomp failed: %s\n", errbuf);
	}

	doc->impl = doc->create(file);
	while (opt_recur || --opt_pages) {
		if (doc->page_next(doc->impl, &page) != 0) {
			page_reset(&page);
			break;
		}
		pageno++;

		if (!(opt_builtin && page.text))
			page.text = ocr(ocr_api, &page);

		char* matches[FIXME_SIZE] = { 0 };
		match(page.text, &pregex, matches, FIXME_SIZE);

		for (int i = 0; i < FIXME_SIZE; i++) {
			if (matches[i]) {
				if (opt_number)
					printf("%d:%s\n", pageno, matches[i]);
				else
					printf("%s\n", matches[i]);
				free(matches[i]);
			}	
		}

		page_reset(&page);
	}

	doc->release(doc->impl);
	regfree(&pregex);
	/* release OCR */
	TessBaseAPIEnd(ocr_api);
	TessBaseAPIDelete(ocr_api);

	return 0;
}
