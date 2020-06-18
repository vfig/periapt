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
