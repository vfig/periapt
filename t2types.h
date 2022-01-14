typedef uint32_t t2id;
typedef float t2float;
typedef unsigned short t2angle;

#define T2_ANGLE_PI 0x8000
#define T2_FLOAT_PI 3.14159265358979323846

typedef struct t2facing {
    t2angle x, y, z;
} t2facing;

typedef struct t2vector {
    t2float x, y, z;
} t2vector;

typedef struct t2location {
   t2vector vec;
   short cell;
   short hint;
} t2location;

typedef struct t2position {
   t2location loc;
   t2facing fac;
} t2position;

typedef struct t2portalcell {
    unsigned char unknown0[6];
    unsigned char flags;
    unsigned char unknown1[61];
    t2vector sphere_center;
    t2float sphere_radius;
} t2portalcell;

typedef struct t2clipdata {
    int l,r,t,b;
    int tl,tr,bl,br;
} t2clipdata;

typedef struct t2mmsmodel {
    char magic[4]; // "LGMM"
    uint32_t version; // 1 or 2
    float radius;
    uint32_t flags;
    uint32_t app_data;
    uint8_t layout;
    uint8_t segs;
    uint8_t smatrs;
    uint8_t smatsegs;
    uint16_t pgons;
    uint16_t verts;
    uint16_t weights;
    uint16_t pad;
    uint32_t map_off;
    uint32_t seg_off;
    uint32_t smatr_off;
    uint32_t smatseg_off;
    uint32_t pgon_off;
    uint32_t norm_off;
    uint32_t vert_vec_off;
    uint32_t vert_uvn_off;
    uint32_t weight_off;
} t2mmsmodel;

typedef struct t2smatr_v1 {
    char name[16];
    uint32_t handle;
    float uv;
    uint8_t mat_type;
    uint8_t smatsegs;
    uint8_t map_start;
    uint8_t flags;
    uint16_t pgons;
    uint16_t pgon_start;
    uint16_t verts;
    uint16_t vert_start;
    uint16_t weight_start;
    uint16_t pad0;
} t2smatr_v1; /* 40 bytes */

typedef struct t2smatr_v2 {
    char name[16];
    uint32_t caps;
    float alpha;
    float self_illum;
    uint32_t pad0;
    uint32_t handle;
    float uv;
    uint8_t mat_type;
    uint8_t smatsegs;
    uint8_t map_start;
    uint8_t flags;
    uint16_t pgons;
    uint16_t pgon_start;
    uint16_t verts;
    uint16_t vert_start;
    uint16_t weight_start;
    uint16_t pad1;
} t2smatr_v2; /* 56 bytes */

typedef struct t2material {
    int32_t cache_index;
    /* i dont care about the rest */
} t2material;

typedef struct t2cachedmaterial {
    IDirect3DBaseTexture9 *d3d_texture;
    uint32_t pad0;
    uint32_t handle; // t2material*
    uint8_t pad1;
    uint8_t pad2;
    uint8_t pad3; // seems to be flags, but i dont care.
    uint8_t pad4;
} t2cachedmaterial;
