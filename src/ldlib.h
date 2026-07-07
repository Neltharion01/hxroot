typedef struct {
    char *buf;
    size_t bufpos;
    size_t bufsiz;
} HxLdlib;

void HxLdlib_set(HxLdlib *ldlib, char *s);
void HxLdlib_append(HxLdlib *ldlib, char *s);
void HxDestroyLdlib(HxLdlib *ldlib);
