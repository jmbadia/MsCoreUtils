/* Johannes Rainer
 */

#include "MsCoreUtils.h"

#include <R.h>
#include <Rinternals.h>
#include <math.h>
#include <Rmath.h>

/**
 * Outer join function. This function is optimized for increasingly ordered
 * input arrays.
 * @param x: sorted `numeric`.
 * @param y: sorted `numeric`.
 * @param tolerance: `numeric` of length equal `length(x)` with accepted
 *        difference
**/
SEXP C_join_outer(SEXP x, SEXP y, SEXP tolerance) {
    int lx, ly, idx, *ptresx, *ptresy, xi, yi, xi_next, yi_next;
    double *px, *py, *tol, idiff, xdiff, ydiff;
    SEXP resx, resy, output, names, tresx, tresy;
    lx = LENGTH(x);
    ly = LENGTH(y);
    px = REAL(x);
    py = REAL(y);
    tol = REAL(tolerance);
    if (lx != LENGTH(tolerance))
        error("'tolerance' has to be of length 1 or length equal to 'length(x)'");
    PROTECT(resx = allocVector(INTSXP, lx + ly));
    PROTECT(resy = allocVector(INTSXP, lx + ly));
    int *presx = INTEGER(resx);
    int *presy = INTEGER(resy);
    idx = -1;
    xi = 0;
    yi = 0;
    xi_next = 0;
    yi_next = 0;
    int i_path = 0;             // 1 incremented y, -1 incremented x, 0 no.
    int xi_last = -1;
    int yi_last = -1;

    while (xi < lx || yi < ly) {
        idx++;
        if (xi >= lx) {
            yi++;
            presx[idx] = NA_INTEGER;
            presy[idx] = yi;
            continue;
        }
        if (yi >= ly) {
            xi++;
            presx[idx] = xi;
            presy[idx] = NA_INTEGER;
            continue;
        }
        // Compare elements
        idiff = fabs(px[xi] - py[yi]);
        if (idiff <= tol[xi]) {
            // Possible matching pair. Need to look ahead if any of the
            // next elements have a smaller distance to ensure we're matching
            // always the best, not the first, match.
            // issue #66: if we're incrementing x to find better matches and
            // then change to increment y (because of a better match) we habe
            // to add also that match - otherwise we would miss a match. See
            // issue #66 for details and examples.
            xi_next = xi + 1;
            yi_next = yi + 1;
            presx[idx] = xi_next;
            presy[idx] = yi_next;
            if (xi_next < lx) xdiff = fabs(px[xi_next] - py[yi]);
            else xdiff = R_PosInf;
            if (yi_next < ly) ydiff = fabs(px[xi] - py[(yi_next)]);
            else ydiff = R_PosInf;
            if (xdiff < idiff || ydiff < idiff) {
                if (xdiff < ydiff) {
                    xi = xi_next;
                    if (i_path > 0) {
                        // issue #66
                        idx--;
                        presx[idx] = xi_next;
                    }
                    else presy[idx] = NA_INTEGER;
                    i_path = -1;
                }
                else {
                    yi = yi_next;
                    if (i_path < 0) {
                        // issue #66
                        idx--;
                        presy[idx] = yi_next;
                    }
                    else presx[idx] = NA_INTEGER;
                    i_path = 1;
                }
            } else {
                i_path = 0;
                xi = xi_next;
                yi = yi_next;
            }
        } else {
            i_path = 0;
            // Decide whether to increment i or j: in the outer join matches are
            // expected to be ordered by value, thus, if x[i] < y[j] we add i
            // to the result and increment it.
            if (px[xi] < py[yi]) {
                xi++;
                presx[idx] = xi;
                presy[idx] = NA_INTEGER;
            } else {
                yi++;
                presx[idx] = NA_INTEGER;
                presy[idx] = yi;
            }
        }
    }

    // Truncate the output vector - there might be a better implementation...
    idx++;
    PROTECT(tresx = allocVector(INTSXP, idx));
    PROTECT(tresy = allocVector(INTSXP, idx));
    ptresx = INTEGER(tresx);
    ptresy = INTEGER(tresy);
    /* memcpy(presx, ptresx, idx * sizeof(int)); */
    /* memcpy(presy, ptresy, idx * sizeof(int)); */
    for (xi = 0; xi <= idx; xi++) {
        ptresx[xi] = presx[xi];
        ptresy[xi] = presy[xi];
    }

    PROTECT(output = allocVector(VECSXP, 2));
    PROTECT(names = allocVector(STRSXP, 2));
    SET_VECTOR_ELT(output, 0, tresx);
    SET_VECTOR_ELT(output, 1, tresy);
    SET_STRING_ELT(names, 0, mkChar("x"));
    SET_STRING_ELT(names, 1, mkChar("y"));
    setAttrib(output, R_NamesSymbol, names);

    UNPROTECT(6);
    return output;
}

/**
 * Left join optimized for increasingly ordered arrays.
 */
SEXP C_join_left(SEXP x, SEXP y, SEXP tolerance) {
    int lx, ly;
    double *px, *py, *tol;
    SEXP resx, resy, output, names;
    lx = LENGTH(x);
    ly = LENGTH(y);
    px = REAL(x);
    py = REAL(y);
    tol = REAL(tolerance);
    if (lx != LENGTH(tolerance))
        error("'tolerance' has to be of length 1 or length equal to 'length(x)'");
    PROTECT(resx = allocVector(INTSXP, lx));
    PROTECT(resy = allocVector(INTSXP, lx));
    int *presx = INTEGER(resx);
    int *presy = INTEGER(resy);

    double xdiff;
    double ydiff;
    double idiff;

    int xi = 0;
    int yi = 0;
    int xi_next = 0;
    int yi_next = 0;
    int yi_last_used = -1;
    int xi_last_used = R_PosInf;

    while (xi < lx) {
        xi_next = xi + 1;
        if (yi < ly) {
            yi_next = yi + 1;
            // Difference for current pair.
            idiff = fabs(px[xi] - py[yi]);
            // Difference for next pairs.
            if (xi_next < lx) xdiff = fabs(px[xi_next] - py[yi]);
            else xdiff = R_PosInf;
            if (yi_next < ly) ydiff = fabs(px[xi] - py[yi_next]);
            else ydiff = R_PosInf;
            // Do we have an acceptable match?
            if (idiff <= tol[xi]) {
                presx[xi] = xi_next;
                presy[xi] = yi_next;
                // Remove last hit with same yi if we incremented xi and will NOT
                // increment yi next.
                if (yi == yi_last_used && xi > xi_last_used) {
                    // We're not incrementing yi next round.
                    if (ydiff > idiff || ydiff > xdiff)
                        presy[xi_last_used] = NA_INTEGER;
                }
                yi_last_used = yi;
                xi_last_used = xi;
            } else {
                presx[xi] = xi_next;
                presy[xi] = NA_INTEGER;
            }
            // Decide which index to increment.
            if (xdiff < idiff || ydiff < idiff) {
                // Increment the index with the smaller distance
                if (xdiff < ydiff) xi = xi_next;
                else yi = yi_next;
            } else {
                // Neither xdiff nor ydiff better than idiff, increment both
                yi = yi_next;
                xi = xi_next;
            }
        } else {
            // Just fill-up the result.
            presy[xi] = NA_INTEGER;
            presx[xi] = xi_next;
            xi = xi_next;
        }
    }

    PROTECT(output = allocVector(VECSXP, 2));
    PROTECT(names = allocVector(STRSXP, 2));
    SET_VECTOR_ELT(output, 0, resx);
    SET_VECTOR_ELT(output, 1, resy);
    SET_STRING_ELT(names, 0, mkChar("x"));
    SET_STRING_ELT(names, 1, mkChar("y"));
    setAttrib(output, R_NamesSymbol, names);

    UNPROTECT(4);
    return output;
}
