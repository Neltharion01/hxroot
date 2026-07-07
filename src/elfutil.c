#include <elf.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "hxroot.h"
#include "elfutil.h"

static int HxReadRpath64(int fd, Elf64_Ehdr *ehdr, Elf64_Shdr *sections, const char *shstrtab, struct HxElfInfo *out) {
    if(ehdr->e_phnum * sizeof(Elf64_Phdr) > 4096) {
        // We aren't going to allocate that much
        errno = E2BIG;
        return -1;
    }
    Elf64_Phdr phdrs[ehdr->e_phnum];

    ssize_t nr = pread(fd, phdrs, sizeof(Elf64_Phdr) * ehdr->e_phnum, ehdr->e_phoff);
    if(nr == -1) return -1;
    if(nr != sizeof(Elf64_Phdr) * ehdr->e_phnum) { errno = ENODATA; return -1; }

    Elf64_Phdr *dyn_phdr = NULL;
    for(int i = 0; i < ehdr->e_phnum; i++) {
        if(phdrs[i].p_type == PT_DYNAMIC) {
            dyn_phdr = &phdrs[i];
            break;
        }
    }
    if(!dyn_phdr) return 0;

    if(dyn_phdr->p_filesz > 4096) {
        errno = E2BIG;
        return -1;
    }

    Elf64_Dyn dyns[dyn_phdr->p_filesz / sizeof(Elf64_Dyn)];
    nr = pread(fd, dyns, dyn_phdr->p_filesz, dyn_phdr->p_offset);
    if(nr == -1) return -1;
    if(nr != dyn_phdr->p_filesz) { errno = ENODATA; return -1; }

    Elf64_Shdr *dynstr = NULL;
    for(int i = 0; i < ehdr->e_shnum; i++) {
        const char *name = shstrtab + sections[i].sh_name;
        if(strcmp(name, ".dynstr") == 0) {
            dynstr = &sections[i];
            break;
        }
    }
    if(!dynstr) return 0;

    Elf64_Off stroff = dynstr->sh_offset;
    Elf64_Xword strsz = dynstr->sh_size;

    // 100MB
    if(strsz > 1024*1024*100) {
        errno = E2BIG;
        return -1;
    }

    AUTO_FREE_CHAR char *strtab = malloc(strsz);

    nr = pread(fd, strtab, strsz, stroff);
    if(nr == -1) return -1;
    if(nr != strsz) { errno = ENODATA; return -1; }

    for(Elf64_Dyn *d = dyns; d->d_tag != DT_NULL; d++) {
        if(d->d_tag == DT_RUNPATH) {
            int off = d->d_un.d_val;
            if(off >= strsz) { errno = E2BIG; return -1; }
            int maxlen = strsz - off - 1;
            int len = strnlen(strtab + off, maxlen) + 1;
            out->runpath = malloc(len);
            memcpy(out->runpath, strtab + off, len);
            // Unlikely case if strtab was malformed, to keep it a valid string
            if(out->runpath[len-1]) out->runpath[len-1] = '\0';
        } else if(d->d_tag == DT_RPATH) {
            int off = d->d_un.d_val;
            if(off >= strsz) { errno = E2BIG; return -1; }
            int maxlen = strsz - off - 1;
            int len = strnlen(strtab + off, maxlen) + 1;
            out->rpath = malloc(len);
            memcpy(out->rpath, strtab + off, len);
            if(out->rpath[len-1]) out->rpath[len-1] = '\0';
        }
    }

    return 0;
}

static int HxReadInterp64(int fd, Elf64_Ehdr *ehdr, Elf64_Shdr *sections, const char *shstrtab, struct HxElfInfo *out) {
    Elf64_Shdr *interp = NULL;

    for(int i = 0; i < ehdr->e_shnum; i++) {
        const char *name = shstrtab + sections[i].sh_name;
        if(strcmp(name, ".interp") == 0) {
            interp = &sections[i];
            break;
        }
    }

    if(interp) {
        if(interp->sh_size > 4096) {
            errno = E2BIG;
            return -1;
        }
        out->interp = malloc(interp->sh_size+1);
        ssize_t nr = pread(fd, out->interp, interp->sh_size, interp->sh_offset);
        if(nr == -1) return -1;
        if(nr != interp->sh_size) { errno = ENODATA; return -1; }
        out->interp[interp->sh_size] = '\0';
        return 1;
    }

    return 0;
}

PRIVATE int HxParseElfInfo64(int fd, Elf64_Ehdr *ehdr, struct HxElfInfo *out) {
    if(ehdr->e_shnum * sizeof(Elf64_Shdr) > 4096) {
        // We aren't going to allocate that much
        errno = E2BIG;
        return -1;
    }

    Elf64_Shdr sections[ehdr->e_shnum];

    ssize_t nr = pread(fd, sections, sizeof(Elf64_Shdr) * ehdr->e_shnum, ehdr->e_shoff);
    if(nr == -1) return -1;
    if(nr != sizeof(Elf64_Shdr) * ehdr->e_shnum) { errno = ENODATA; return -1; }

    Elf64_Shdr *strtab = &sections[ehdr->e_shstrndx];

    if(strtab->sh_size > 4096) {
        errno = E2BIG;
        return -1;
    }
    char shstrtab[strtab->sh_size];

    nr = pread(fd, shstrtab, strtab->sh_size, strtab->sh_offset);
    if(nr == -1) return -1;
    if(nr != strtab->sh_size) { errno = ENODATA; return -1; }

    if(HxReadInterp64(fd, ehdr, sections, shstrtab, out) == -1) return -1;
    if(HxReadRpath64(fd, ehdr, sections, shstrtab, out) == -1) return -1;
    return 0;
}

PRIVATE void HxDestroyElfInfo(struct HxElfInfo *info) {
    if(info->interp) {
        free(info->interp);
        info->interp = NULL;
    }
    if(info->rpath) {
        free(info->rpath);
        info->rpath = NULL;
    }
    if(info->runpath) {
        free(info->runpath);
        info->runpath = NULL;
    }
}
