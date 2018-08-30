#ifndef P2P_ENVIRONMENT_H

#define P2P_ENVIRONMENT_H

#include <stdlib.h>
#include <string.h>

#define P2P_ENVIR_DELARE_INT(name) \
    int name ; \
    int get_##name(){return name;};\
    void set_##name(int v){name=v;};

#define P2P_EVNIR_DELARE_STR(name) \
    char* name;\
    char* get_##name(){return name;}\
    void  set_##name(const char* v){if(name) free(name);name=strdup(v);}

typedef struct _p2p_envir_t {

    P2P_ENVIR_DELARE_INT(flag);
    P2P_EVNIR_DELARE_STR(devid);

} p2p_envir_t;

#endif /* end of include guard: P2P_ENVIRONMENT_H */
