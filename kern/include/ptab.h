#pragma once

#include "types.h"

class Ptab
{
    public:
        static void insert_mapping(mword virt, mword phys, mword attr);

        [[nodiscard]]
        static void *remap(mword addr);

    private:
        [[nodiscard]]
        static mword *get_pd(mword virt, mword attr);
};
