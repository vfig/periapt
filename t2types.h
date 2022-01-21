typedef unsigned int t2id;
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
    unsigned int version; // 1 or 2
    float radius;
    unsigned int flags;
    unsigned int app_data;
    unsigned char layout;
    unsigned char segs;
    unsigned char smatrs;
    unsigned char smatsegs;
    unsigned short pgons;
    unsigned short verts;
    unsigned short weights;
    unsigned short pad;
    unsigned int map_off;
    unsigned int seg_off;
    unsigned int smatr_off;
    unsigned int smatseg_off;
    unsigned int pgon_off;
    unsigned int norm_off;
    unsigned int vert_vec_off;
    unsigned int vert_uvn_off;
    unsigned int weight_off;
} t2mmsmodel;

typedef struct t2smatr_v1 {
    char name[16];
    unsigned int handle;
    float uv;
    unsigned char mat_type;
    unsigned char smatsegs;
    unsigned char map_start;
    unsigned char flags;
    unsigned short pgons;
    unsigned short pgon_start;
    unsigned short verts;
    unsigned short vert_start;
    unsigned short weight_start;
    unsigned short pad0;
} t2smatr_v1; /* 40 bytes */

typedef struct t2smatr_v2 {
    char name[16];
    unsigned int caps;
    float alpha;
    float self_illum;
    unsigned int pad0;
    unsigned int handle;
    float uv;
    unsigned char mat_type;
    unsigned char smatsegs;
    unsigned char map_start;
    unsigned char flags;
    unsigned short pgons;
    unsigned short pgon_start;
    unsigned short verts;
    unsigned short vert_start;
    unsigned short weight_start;
    unsigned short pad1;
} t2smatr_v2; /* 56 bytes */

typedef struct t2material {
    int cache_index;
    /* i dont care about the rest */
} t2material;

typedef struct t2cachedmaterial {
    IDirect3DBaseTexture9 *d3d_texture;
    unsigned int pad0;
    unsigned int handle; // t2material*
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3; // seems to be flags, but i dont care.
    unsigned char pad4;
} t2cachedmaterial;
