struct HxElfInfo {
    char *interp, *rpath, *runpath;
};

int HxParseElfInfo64(int fd, Elf64_Ehdr *ehdr, struct HxElfInfo *out);
void HxDestroyElfInfo(struct HxElfInfo *info);
