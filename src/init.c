#include "MsCoreUtils.h"

static const R_CallMethodDef CallEntries[] = {
    {"C_closest_dup_keep", (DL_FUNC) &C_closest_dup_keep, 4},
    {"C_closest_dup_closest", (DL_FUNC) &C_closest_dup_closest, 4},
    {"C_closest_dup_remove", (DL_FUNC) &C_closest_dup_remove, 4},
    {"C_impNeighbourAvg", (DL_FUNC) &C_impNeighbourAvg, 2},
    {"C_join_outer", (DL_FUNC) &C_join_outer, 3},
    {"C_join_outer2", (DL_FUNC) &C_join_outer2, 4},
    {"C_join_left", (DL_FUNC) &C_join_left, 3},
    {"C_join_left2", (DL_FUNC) &C_join_left2, 4},
    {"C_join_inner2", (DL_FUNC) &C_join_inner2, 4},
    {"C_localMaxima", (DL_FUNC) &C_localMaxima, 2},
    {NULL, NULL, 0}
};

void R_init_MsCoreUtils(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}