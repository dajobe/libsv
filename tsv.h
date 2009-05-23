/* tsv.c */
typedef struct tsv_s tsv;
typedef int (*tsv_fields_callback)(tsv *t, void *user_data, char** fields, size_t *widths, size_t count);

  
tsv* tsv_init(FILE* fh, void *user_data, tsv_fields_callback callback);
void tsv_free(tsv *t);

int tsv_get_line(tsv *t);

int tsv_parse_chunk(tsv *t, char *buffer, size_t len);

