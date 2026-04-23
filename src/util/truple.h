#ifndef SHADERTASTIC_TRUPLE_H
#define SHADERTASTIC_TRUPLE_H

struct float3 {
    union {
        struct {
            float x;
            float y;
            float z;
        };
        float ptr[3];
    };
};

#endif // SHADERTASTIC_TRUPLE_H
