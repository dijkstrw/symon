#ifndef _SYMON_DISKNAME_H
#define _SYMON_DISKNAME_H

struct disknamectx
{
    const char *spath;
    char *dpath;
    size_t maxlen;
    unsigned int i;
    unsigned int link;
};

void initdisknamectx(struct disknamectx *c, const char *spath, char *dpath, size_t dmaxlen);
char *nextdiskname(struct disknamectx *c);

#endif /* _SYMON_DISKNAME_H */
