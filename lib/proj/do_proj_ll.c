
/**
   \file do_proj.c

   \brief GProj library - Functions for re-projecting point data

   \author Original Author unknown, probably Soil Conservation Service
   Eric Miller, Paul Kelly

   (C) 2003-2008 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
**/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

static double METERS_in = 1.0;

#ifdef HAVE_PROJ_H
static char *gpj_get_def(PJ *P)
{
    char *pjdef;
    PJ_PROJ_INFO pj_proj_info = proj_pj_info(P);

    pjdef = G_store(pj_proj_info.definition);

    return pjdef;
}

/** 
 * \brief Re-project a point between a co-ordinate system and its 
 *        latitude / longitude equivalent, using the same datum, 
 *        ellipsoid and datum transformation
 * 
 * This function takes a pointer to a pj_info structure as argument, 
 * and projects a point between the co-ordinate system and its 
 * latitude / longitude equivalent. With forward projection, the point 
 * is projected from the co-ordinate system to its latitude / longitude 
 * equivalent, while backward projection does the reverse.
 * The easting and northing of the point are contained in two pointers 
 * passed to the function; these will be overwritten by the co-ordinates
 * of the re-projected point.
 * 
 * \param x Pointer to a double containing easting or longitude
 * \param y Pointer to a double containing northing or latitude
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param direction direction of re-projection (PJ_FWD or PJ_INV)
 * 
 * \return Return value from PROJ proj_trans() function
 **/

int GPJ_do_proj_ll(double *x, double *y,
	          const struct pj_info *info_in, int direction)
{
    int ok;
    PJ *P;
    char *projdef;
    PJ_COORD c;

    ok = 0;
    if (strncmp(info_in->proj, "ll", 2) == 0) {
	/* nothing to do */
	return ok;
    }

    projdef = gpj_get_def(info_in->pj);
    P = proj_create(PJ_DEFAULT_CTX, projdef);
    if (P == NULL)
	G_fatal_error(_("proj_create() failed"));
    G_free(projdef);

    METERS_in = info_in->meters;

    if (direction == PJ_FWD) {
	/* from ll to projected */

	/* convert to radians */
	c.lpzt.lam = (*x) / RAD_TO_DEG;
	c.lpzt.phi = (*y) / RAD_TO_DEG;
	c.lpzt.z = 0;
	c.lpzt.t = 0;

	c = proj_trans(P, PJ_FWD, c);
	ok = proj_errno(P);

	/* convert to map units */
	*x = c.xy.x / METERS_in;
	*y = c.xy.y / METERS_in;
    }
    else {
	/* from projected to ll */

	/* convert to meters */
	c.xyzt.x = *x * METERS_in;
	c.xyzt.y = *y * METERS_in;
	c.xyzt.z = 0;
	c.xyzt.t = 0;

	c = proj_trans(P, PJ_INV, c);
	ok = proj_errno(P);

	/* convert to degrees */
	*x = c.lp.lam * RAD_TO_DEG;
	*y = c.lp.phi * RAD_TO_DEG;
    }
    proj_destroy(P);

    if (ok < 0) {
	G_warning(_("proj_trans() failed: %d"), ok);
    }
    return ok;
}

/** 
 * \brief Re-project an array of points between a co-ordinate system 
 *        and its latitude / longitude equivalent, using the same 
 *        datum, ellipsoid and datum transformation
 * 
 * This function takes a pointer to a pj_info structure as argument, 
 * and projects an array of points between the co-ordinate system and its 
 * latitude / longitude equivalent. With forward projection, the points 
 * are projected from the co-ordinate system to its latitude / longitude 
 * equivalent, while backward projection does the reverse.
 * The easting and northing of the points are contained in two pointers passed 
 * to the function; these will be overwritten by the co-ordinates of the 
 * re-projected point.
 * 
 * \param count Number of points in the arrays to be transformed
 * \param x Pointer to an array of type double containing easting or longitude
 * \param y Pointer to an array of type double containing northing or latitude
 * \param h Pointer to an array of type double containing ellipsoidal height. 
 *          May be null in which case a two-dimensional re-projection will be 
 *          done
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param direction direction of re-projection (PJ_FWD or PJ_INV)
 * 
 * \return Return value from PROJ proj_trans() function
 **/

int GPJ_do_transform_ll(int count, double *x, double *y, double *h,
		       const struct pj_info *info_in, int direction)
{
    int ok;
    int has_h = 1;
    int i;
    PJ *P;
    char *projdef;
    PJ_COORD c;

    ok = 0;
    if (strncmp(info_in->proj, "ll", 2) == 0) {
	/* nothing to do */
	return ok;
    }

    projdef = gpj_get_def(info_in->pj);
    P = proj_create(PJ_DEFAULT_CTX, projdef);
    if (P == NULL)
	G_fatal_error(_("proj_create() failed"));
    G_free(projdef);

    METERS_in = info_in->meters;

    if (h == NULL) {
	h = G_malloc(sizeof *h * count);
	/* they say memset is only guaranteed for chars ;-( */
	for (i = 0; i < count; ++i)
	    h[i] = 0.0;
	has_h = 0;
    }

    if (direction == PJ_FWD) {
	/* from ll to projected */

	for (i = 0; i < count; i++) {
	    /* convert to radians */
	    c.lpzt.lam = x[i] / RAD_TO_DEG;
	    c.lpzt.phi = y[i] / RAD_TO_DEG;
	    c.lpzt.z = h[i];
	    c.lpzt.t = 0;

	    c = proj_trans(P, PJ_FWD, c);
	    if ((ok = proj_errno(P)) < 0)
		break;

	    /* convert to map units */
	    x[i] = c.xy.x / METERS_in;
	    y[i] = c.xy.y / METERS_in;
	}
    }
    else {
	/* from projected to ll */

	for (i = 0; i < count; i++) {
	    /* convert to meters */
	    c.xyzt.x = x[i] * METERS_in;
	    c.xyzt.y = y[i] * METERS_in;
	    c.xyzt.z = h[i];
	    c.xyzt.t = 0;

	    c = proj_trans(P, PJ_INV, c);
	    if ((ok = proj_errno(P)) < 0)
		break;

	    /* convert to degrees */
	    x[i] = c.lp.lam * RAD_TO_DEG;
	    y[i] = c.lp.phi * RAD_TO_DEG;
	}
    }
    if (!has_h)
	G_free(h);
    proj_destroy(P);

    if (ok < 0) {
	G_warning(_("proj_trans() failed: %d"), ok);
    }
    return ok;
}

#endif /* HAVE_PROJ_H */

