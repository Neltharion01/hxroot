void eprintf(char *fmt, ...);
void HxExpandPath_r(char *dest, const char *path);
char *HxExpandPath(const char *path);
char *HxExpandPath2(const char *path);
void HxUnexpandPath(char *path);
void HxAutoCloseFd(int *fd);
