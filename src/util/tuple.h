#ifndef SHADERTASTIC_TUPLE_H
#define SHADERTASTIC_TUPLE_H

struct int2 {
    union {
        struct {
            int x;
            int y;
        };
        int ptr[2];
    };
};
struct uint2 {
    union {
        struct {
            unsigned int x;
            unsigned int y;
        };
        unsigned int ptr[2];
    };
};
struct float2 {
    union {
        struct {
            float x;
            float y;
        };
        float ptr[2];
    };
};
struct double2 {
    union {
        struct {
            double x;
            double y;
        };
        double ptr[2];
    };
};

#endif // SHADERTASTIC_TUPLE_H
