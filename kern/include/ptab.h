#pragma once

#include "cpu.h"
#include "types.h"

class Ptab
{
    public:
        static void insert_mapping(mword virt, mword phys, mword attr);

        static void *remap(mword addr);

    private:
        static mword *get_pd(mword virt, mword attr);
};
