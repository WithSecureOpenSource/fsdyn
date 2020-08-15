#pragma once
static __attribute__((constructor)) void FSDYN_VERSION()
{
    extern const char *fsdyn_version_tag;
    if (!*fsdyn_version_tag)
        fsdyn_version_tag++;
}
