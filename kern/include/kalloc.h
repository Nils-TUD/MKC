#pragma once

#include "types.h"

class Kalloc
{
    private:
        mword begin, end;

    public:
        enum Fill
        {
            NOFILL = 0,
            FILL_0,
            FILL_1
        };

        static Kalloc allocator;

        Kalloc(mword virt_begin, mword virt_end) : begin(virt_begin), end(virt_end) {}

        [[nodiscard]]
        void *alloc(unsigned size);

        [[nodiscard]]
        void *alloc_page(unsigned size, Fill fill = NOFILL);

        [[nodiscard]]
        static void *phys2virt(mword);

        [[nodiscard]]
        static mword virt2phys(void *);
};
