#pragma once
static __attribute__((constructor)) void AVLTREE_VERSION()
{
    extern const char *avltree_version_tag;
    if (!*avltree_version_tag)
        avltree_version_tag++;
}
