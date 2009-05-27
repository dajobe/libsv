/* tsv.c */

typedef enum {
  TSV_STATUS_OK = 0,
  TSV_STATUS_FAILED,
  TSV_STATUS_NO_MEMORY
} tsv_status_t;
  

typedef struct tsv_s tsv;
typedef tsv_status_t (*tsv_fields_callback)(tsv *t, void *user_data, char** fields, size_t *widths, size_t count);

/* bit flags for tsv_init() */
#define TSV_FLAGS_SAVE_HEADER (1<<0)

tsv* tsv_init(void *user_data, tsv_fields_callback callback, int flags);
void tsv_free(tsv *t);

int tsv_get_line(tsv *t);
const char* tsv_get_header(tsv *t, unsigned int i, size_t *width_p);

tsv_status_t tsv_parse_chunk(tsv *t, char *buffer, size_t len);

