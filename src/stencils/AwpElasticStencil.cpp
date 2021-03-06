/*****************************************************************************

YASK: Yet Another Stencil Kernel
Copyright (c) 2014-2018, Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

*****************************************************************************/

// Stencil equations for AWP elastic* numerics.
// *This version does not contain the time-varying attenuation memory grids
// or the related attenuation constant grids.
// This version also contains some experimental code for calculating the
// free-surface boundary values.
// http://hpgeoc.sdsc.edu/AWPODC
// http://www.sdsc.edu/News%20Items/PR20160209_earthquake_center.html

// Set the following macro to use a sponge grid instead of 3 sponge arrays.
//#define FULL_SPONGE_GRID

// Set the following macro to define all points, even those above the
// surface that are never used.
#define SET_ALL_POINTS

// Set the following macro to calculate free-surface boundary values.
#define DO_ABOVE_SURFACE

// Set the following macro to use intermediate scratch grids.
//#define USE_SCRATCH_GRIDS

#include "Soln.hpp"

class AwpElasticStencil : public StencilBase {

protected:

    // Indices & dimensions.
    MAKE_STEP_INDEX(t);           // step in time dim.
    MAKE_DOMAIN_INDEX(x);         // spatial dim.
    MAKE_DOMAIN_INDEX(y);         // spatial dim.
    MAKE_DOMAIN_INDEX(z);         // spatial dim.

    // Time-varying 3D-spatial velocity grids.
    MAKE_GRID(vel_x, t, x, y, z);
    MAKE_GRID(vel_y, t, x, y, z);
    MAKE_GRID(vel_z, t, x, y, z);
        
    // Time-varying 3D-spatial Stress grids.
    MAKE_GRID(stress_xx, t, x, y, z);
    MAKE_GRID(stress_yy, t, x, y, z);
    MAKE_GRID(stress_zz, t, x, y, z);
    MAKE_GRID(stress_xy, t, x, y, z);
    MAKE_GRID(stress_xz, t, x, y, z);
    MAKE_GRID(stress_yz, t, x, y, z);

    // 3D-spatial Lame' coefficients.
    MAKE_GRID(lambda, x, y, z);
    MAKE_GRID(rho, x, y, z);
    MAKE_GRID(mu, x, y, z);

    // Sponge coefficients.
    // (Most of these will be 1.0.)
#ifdef FULL_SPONGE_GRID
    MAKE_GRID(sponge, x, y, z);
#else
    MAKE_ARRAY(cr_x, x);
    MAKE_ARRAY(cr_y, y);
    MAKE_ARRAY(cr_z, z);
#endif

    // Spatial FD coefficients.
    const double c1 = 9.0/8.0;
    const double c2 = -1.0/24.0;

    // Physical dimensions in time and space.
    MAKE_SCALAR(delta_t);
    MAKE_SCALAR(h);

    // For the surface stress conditions, we need to write into 2 points
    // above the surface.  Since we can only write into the "domain", we
    // will define the surface index to be 2 points before the last domain
    // index. Thus, there will be two layers in the domain above the surface.
#define SURFACE_IDX (last_index(z) - 2)
    
    // Define some sub-domains related to the surface.
#define IF_BELOW_SURFACE IF (z < SURFACE_IDX)
#define IF_AT_SURFACE IF (z == SURFACE_IDX)
#define IF_AT_OR_BELOW_SURFACE IF (z <= SURFACE_IDX)
#define IF_ONE_ABOVE_SURFACE IF (z == SURFACE_IDX + 1)
#define IF_TWO_ABOVE_SURFACE IF (z == SURFACE_IDX + 2)

#ifdef USE_SCRATCH_GRIDS
        MAKE_SCRATCH_GRID(tmp_vel_x, x, y, z);
        MAKE_SCRATCH_GRID(tmp_vel_y, x, y, z);
        MAKE_SCRATCH_GRID(tmp_vel_z, x, y, z);
#endif
    
public:

    AwpElasticStencil(StencilList& stencils) :
        StencilBase("awp_elastic", stencils) { }

    // Adjustment for sponge layer.
    void adjust_for_sponge(GridValue& val) {

#ifdef FULL_SPONGE_GRID
        val *= sponge(x, y, z);
#else
        val *= cr_x(x) * cr_y(y) * cr_z(z);
#endif        
    }

    // Velocity-grid define functions.  For each D in x, y, z, define vel_D
    // at t+1 based on vel_x at t and stress grids at t.  Note that the t,
    // x, y, z parameters are integer grid indices, not actual offsets in
    // time or space, so half-steps due to staggered grids are adjusted
    // appropriately.

    GridValue get_next_vel_x(GridIndex x, GridIndex y, GridIndex z) {
        GridValue rho_val = (rho(x, y,   z  ) +
                             rho(x, y-1, z  ) +
                             rho(x, y,   z-1) +
                             rho(x, y-1, z-1)) * (1.0 / 4.0);
        GridValue d_val =
            c1 * (stress_xx(t, x,   y,   z  ) - stress_xx(t, x-1, y,   z  )) +
            c2 * (stress_xx(t, x+1, y,   z  ) - stress_xx(t, x-2, y,   z  )) +
            c1 * (stress_xy(t, x,   y,   z  ) - stress_xy(t, x,   y-1, z  )) +
            c2 * (stress_xy(t, x,   y+1, z  ) - stress_xy(t, x,   y-2, z  )) +
            c1 * (stress_xz(t, x,   y,   z  ) - stress_xz(t, x,   y,   z-1)) +
            c2 * (stress_xz(t, x,   y,   z+1) - stress_xz(t, x,   y,   z-2));
        GridValue next_vel_x = vel_x(t, x, y, z) + (delta_t / (h * rho_val)) * d_val;
        adjust_for_sponge(next_vel_x);

        // Return the value at t+1.
        return next_vel_x;
    }
    GridValue get_next_vel_y(GridIndex x, GridIndex y, GridIndex z) {
        GridValue rho_val = (rho(x,   y, z  ) +
                             rho(x+1, y, z  ) +
                             rho(x,   y, z-1) +
                             rho(x+1, y, z-1)) * (1.0 / 4.0);
        GridValue d_val =
            c1 * (stress_xy(t, x+1, y,   z  ) - stress_xy(t, x,   y,   z  )) +
            c2 * (stress_xy(t, x+2, y,   z  ) - stress_xy(t, x-1, y,   z  )) +
            c1 * (stress_yy(t, x,   y+1, z  ) - stress_yy(t, x,   y,   z  )) +
            c2 * (stress_yy(t, x,   y+2, z  ) - stress_yy(t, x,   y-1, z  )) +
            c1 * (stress_yz(t, x,   y,   z  ) - stress_yz(t, x,   y,   z-1)) +
            c2 * (stress_yz(t, x,   y,   z+1) - stress_yz(t, x,   y,   z-2));
        GridValue next_vel_y = vel_y(t, x, y, z) + (delta_t / (h * rho_val)) * d_val;
        adjust_for_sponge(next_vel_y);

        // Return the value at t+1.
        return next_vel_y;
    }
    GridValue get_next_vel_z(GridIndex x, GridIndex y, GridIndex z) {
        GridValue rho_val = (rho(x,   y,   z) +
                             rho(x+1, y,   z) +
                             rho(x,   y-1, z) +
                             rho(x+1, y-1, z)) * (1.0 / 4.0);
        GridValue d_val =
            c1 * (stress_xz(t, x+1, y,   z  ) - stress_xz(t, x,   y,   z  )) +
            c2 * (stress_xz(t, x+2, y,   z  ) - stress_xz(t, x-1, y,   z  )) +
            c1 * (stress_yz(t, x,   y,   z  ) - stress_yz(t, x,   y-1, z  )) +
            c2 * (stress_yz(t, x,   y+1, z  ) - stress_yz(t, x,   y-2, z  )) +
            c1 * (stress_zz(t, x,   y,   z+1) - stress_zz(t, x,   y,   z  )) +
            c2 * (stress_zz(t, x,   y,   z+2) - stress_zz(t, x,   y,   z-1));
        GridValue next_vel_z = vel_z(t, x, y, z) + (delta_t / (h * rho_val)) * d_val;
        adjust_for_sponge(next_vel_z);

        // Return the value at t+1.
        return next_vel_z;
    }

    // Free-surface boundary equations for velocity.
    void define_free_surface_vel() {

        // Since we're defining points when z == surface + 1,
        // the surface itself will be at z - 1;
        GridIndex surf = z - 1;

#ifdef USE_SCRATCH_GRIDS

        // The values for velocity at t+1 will be needed
        // in multiple free-surface calculations.
        // Thus, it will reduce the number of FP ops
        // required if we pre-compute them and store them
        // in scratch grids.
#define VEL_X tmp_vel_x
#define VEL_Y tmp_vel_y
#define VEL_Z tmp_vel_z
        VEL_X(x, y, z) EQUALS get_next_vel_x(x, y, z);
        VEL_Y(x, y, z) EQUALS get_next_vel_y(x, y, z);
        VEL_Z(x, y, z) EQUALS get_next_vel_z(x, y, z);

#else

        // If not using scratch grids, just call the
        // functions to calculate each value of velocity
        // at t+1 every time it's needed.
#define VEL_X get_next_vel_x
#define VEL_Y get_next_vel_y
#define VEL_Z get_next_vel_z
#endif

        // A couple of intermediate values.
        GridValue d_x_val = VEL_X(x+1, y, surf) -
            (VEL_Z(x+1, y, surf) - VEL_Z(x, y, surf));
        GridValue d_y_val = VEL_Y(x, y-1, surf) -
            (VEL_Z(x, y, surf) - VEL_Z(x, y-1, surf));
        
        // Following values are valid one layer above the free surface.
        GridValue plus1_vel_x = VEL_X(x, y, surf) -
            (VEL_Z(x, y, surf) - VEL_Z(x-1, y, surf));
        GridValue plus1_vel_y = VEL_Y(x, y, surf) -
            (VEL_Z(x, y+1, surf) - VEL_Z(x, y, surf));
        GridValue plus1_vel_z = VEL_Z(x, y, surf) -
            ((d_x_val - plus1_vel_x) +
             (VEL_X(x+1, y, surf) - VEL_X(x, y, surf)) +
             (plus1_vel_y - d_y_val) +
             (VEL_Y(x, y, surf) - VEL_Y(x, y-1, surf))) /
            ((mu(x, y, surf) *
              (2.0 / mu(x, y, surf) + 1.0 / lambda(x, y, surf))));
#undef VEL_X
#undef VEL_Y
#undef VEL_Z
        
        // Define layer at one point above surface.
        vel_x(t+1, x, y, z) EQUALS plus1_vel_x IF_ONE_ABOVE_SURFACE;
        vel_y(t+1, x, y, z) EQUALS plus1_vel_y IF_ONE_ABOVE_SURFACE;
        vel_z(t+1, x, y, z) EQUALS plus1_vel_z IF_ONE_ABOVE_SURFACE;

#ifdef SET_ALL_POINTS
        // Define layer two points above surface for completeness, even
        // though these aren't input to any stencils.
        vel_x(t+1, x, y, z) EQUALS 0.0 IF_TWO_ABOVE_SURFACE;
        vel_y(t+1, x, y, z) EQUALS 0.0 IF_TWO_ABOVE_SURFACE;
        vel_z(t+1, x, y, z) EQUALS 0.0 IF_TWO_ABOVE_SURFACE;
#endif
    }

    // Compute average of 8 neighbors.
    GridValue ave8(Grid& g, GridIndex x, GridIndex y, GridIndex z) {
        
        return 8.0 /
            (g(x,   y,   z  ) + g(x+1, y,   z  ) +
             g(x,   y-1, z  ) + g(x+1, y-1, z  ) +
             g(x,   y,   z-1) + g(x+1, y,   z-1) +
             g(x,   y-1, z-1) + g(x+1, y-1, z-1));
    }

    // Some common velocity calculations.
    GridValue d_x_val(GridIndex x, GridIndex y, GridIndex z) {
        return
            c1 * (vel_x(t+1, x+1, y,   z  ) - vel_x(t+1, x,   y,   z  )) +
            c2 * (vel_x(t+1, x+2, y,   z  ) - vel_x(t+1, x-1, y,   z  ));
    }
    GridValue d_y_val(GridIndex x, GridIndex y, GridIndex z) {
        return
            c1 * (vel_y(t+1, x,   y,   z  ) - vel_y(t+1, x,   y-1, z  )) +
            c2 * (vel_y(t+1, x,   y+1, z  ) - vel_y(t+1, x,   y-2, z  ));
    }
    GridValue d_z_val(GridIndex x, GridIndex y, GridIndex z) {
        return
            c1 * (vel_z(t+1, x,   y,   z  ) - vel_z(t+1, x,   y,   z-1)) +
            c2 * (vel_z(t+1, x,   y,   z+1) - vel_z(t+1, x,   y,   z-2));
    }
    
    // Stress-grid define functions.  For each D in xx, yy, zz, xy, xz, yz,
    // define stress_D at t+1 based on stress_D at t and vel grids at t+1.
    // This implies that the velocity-grid define functions must be called
    // before these for a given value of t.  Note that the t, x, y, z
    // parameters are integer grid indices, not actual offsets in time or
    // space, so half-steps due to staggered grids are adjusted
    // appropriately.

    GridValue get_next_stress_xx(GridIndex x, GridIndex y, GridIndex z) {

        GridValue next_stress_xx = stress_xx(t, x, y, z) +
            ((delta_t / h) * ((2 * ave8(mu, x, y, z) * d_x_val(x, y, z)) +
                              (ave8(lambda, x, y, z) *
                               (d_x_val(x, y, z) + d_y_val(x, y, z) + d_z_val(x, y, z)))));
        adjust_for_sponge(next_stress_xx);

        // Return the value at t+1.
        return next_stress_xx;
    }
    GridValue get_next_stress_yy(GridIndex x, GridIndex y, GridIndex z) {

        GridValue next_stress_yy = stress_yy(t, x, y, z) +
            ((delta_t / h) * ((2 * ave8(mu, x, y, z) * d_y_val(x, y, z)) +
                              (ave8(lambda, x, y, z) *
                               (d_x_val(x, y, z) + d_y_val(x, y, z) + d_z_val(x, y, z)))));
        adjust_for_sponge(next_stress_yy);

        // Return the value at t+1.
        return next_stress_yy;
    }
    GridValue get_next_stress_zz(GridIndex x, GridIndex y, GridIndex z) {

        GridValue next_stress_zz = stress_zz(t, x, y, z) +
            ((delta_t / h) * ((2 * ave8(mu, x, y, z) * d_z_val(x, y, z)) +
                              (ave8(lambda, x, y, z) *
                               (d_x_val(x, y, z) + d_y_val(x, y, z) + d_z_val(x, y, z)))));
        adjust_for_sponge(next_stress_zz);

        // return the value at t+1.
        return next_stress_zz;
    }
    GridValue get_next_stress_xy(GridIndex x, GridIndex y, GridIndex z) {

        // Compute average of 2 neighbors.
        GridValue mu2 = 2.0 /
            (mu(x,   y,   z  ) + mu(x,   y,   z-1));

        // Note that we are using the velocity values at t+1.
        GridValue d_xy_val =
            c1 * (vel_x(t+1, x,   y+1, z  ) - vel_x(t+1, x,   y,   z  )) +
            c2 * (vel_x(t+1, x,   y+2, z  ) - vel_x(t+1, x,   y-1, z  ));
        GridValue d_yx_val =
            c1 * (vel_y(t+1, x,   y,   z  ) - vel_y(t+1, x-1, y,   z  )) +
            c2 * (vel_y(t+1, x+1, y,   z  ) - vel_y(t+1, x-2, y,   z  ));

        GridValue next_stress_xy = stress_xy(t, x, y, z) +
            ((mu2 * delta_t / h) * (d_xy_val + d_yx_val));
        adjust_for_sponge(next_stress_xy);

        // return the value at t+1.
        return next_stress_xy;
    }
    GridValue get_next_stress_xz(GridIndex x, GridIndex y, GridIndex z) {

        // Compute average of 2 neighbors.
        GridValue mu2 = 2.0 /
            (mu(x,   y,   z  ) + mu(x,   y-1, z  ));

        // Note that we are using the velocity values at t+1.
        GridValue d_xz_val =
            c1 * (vel_x(t+1, x,   y,   z+1) - vel_x(t+1, x,   y,   z  )) +
            c2 * (vel_x(t+1, x,   y,   z+2) - vel_x(t+1, x,   y,   z-1));
        GridValue d_zx_val =
            c1 * (vel_z(t+1, x,   y,   z  ) - vel_z(t+1, x-1, y,   z  )) +
            c2 * (vel_z(t+1, x+1, y,   z  ) - vel_z(t+1, x-2, y,   z  ));

        GridValue next_stress_xz = stress_xz(t, x, y, z) +
            ((mu2 * delta_t / h) * (d_xz_val + d_zx_val));
        adjust_for_sponge(next_stress_xz);

        // return the value at t+1.
        return next_stress_xz;
    }
    GridValue get_next_stress_yz(GridIndex x, GridIndex y, GridIndex z) {

        // Compute average of 2 neighbors.
        GridValue mu2 = 2.0 /
            (mu(x,   y,   z  ) + mu(x+1, y,   z  ));

        // Note that we are using the velocity values at t+1.
        GridValue d_yz_val =
            c1 * (vel_y(t+1, x,   y,   z+1) - vel_y(t+1, x,   y,   z  )) +
            c2 * (vel_y(t+1, x,   y,   z+2) - vel_y(t+1, x,   y,   z-1));
        GridValue d_zy_val =
            c1 * (vel_z(t+1, x,   y+1, z  ) - vel_z(t+1, x,   y,   z  )) +
            c2 * (vel_z(t+1, x,   y+2, z  ) - vel_z(t+1, x,   y-1, z  ));

        GridValue next_stress_yz = stress_yz(t, x, y, z) +
            ((mu2 * delta_t / h) * (d_yz_val + d_zy_val));
        adjust_for_sponge(next_stress_yz);

        // return the value at t+1.
        return next_stress_yz;
    }

    // Free-surface boundary equations for stress.
    void define_free_surface_stress() {

        // When z == surface + 1, the surface will be at z - 1;
        GridIndex surf = z - 1;

        stress_zz(t+1, x, y, z) EQUALS -get_next_stress_zz(x, y, surf) IF_ONE_ABOVE_SURFACE;
        stress_xz(t+1, x, y, z) EQUALS -get_next_stress_xz(x, y, surf-1) IF_ONE_ABOVE_SURFACE;
        stress_yz(t+1, x, y, z) EQUALS -get_next_stress_yz(x, y, surf-1) IF_ONE_ABOVE_SURFACE;

#ifdef SET_ALL_POINTS
        // Define other 3 stress values for completeness, even
        // though these aren't input to any stencils.
        stress_xx(t+1, x, y, z) EQUALS 0.0 IF_ONE_ABOVE_SURFACE;
        stress_yy(t+1, x, y, z) EQUALS 0.0 IF_ONE_ABOVE_SURFACE;
        stress_xy(t+1, x, y, z) EQUALS 0.0 IF_ONE_ABOVE_SURFACE;
#endif
        
        // When z == surface + 2, the surface will be at z - 2;
        surf = z - 2;

        stress_zz(t+1, x, y, z) EQUALS -get_next_stress_zz(x, y, surf-1) IF_TWO_ABOVE_SURFACE;
        stress_xz(t+1, x, y, z) EQUALS -get_next_stress_xz(x, y, surf-2) IF_TWO_ABOVE_SURFACE;
        stress_yz(t+1, x, y, z) EQUALS -get_next_stress_yz(x, y, surf-2) IF_TWO_ABOVE_SURFACE;

#ifdef SET_ALL_POINTS
        // Define other 3 stress values for completeness, even
        // though these aren't input to any stencils.
        stress_xx(t+1, x, y, z) EQUALS 0.0 IF_TWO_ABOVE_SURFACE;
        stress_yy(t+1, x, y, z) EQUALS 0.0 IF_TWO_ABOVE_SURFACE;
        stress_xy(t+1, x, y, z) EQUALS 0.0 IF_TWO_ABOVE_SURFACE;
#endif
    }
    
    // Define the t+1 values for all velocity and stress grids.
    virtual void define() {

        // Define velocity components.
        vel_x(t+1, x, y, z) EQUALS get_next_vel_x(x, y, z) IF_AT_OR_BELOW_SURFACE;
        vel_y(t+1, x, y, z) EQUALS get_next_vel_y(x, y, z) IF_AT_OR_BELOW_SURFACE;
        vel_z(t+1, x, y, z) EQUALS get_next_vel_z(x, y, z) IF_AT_OR_BELOW_SURFACE;

        // Define stress components.  Use non-overlapping sub-domains only,
        // i.e. AT and BELOW but not AT_OR_BELOW, even though there are some
        // repeated stencils. This allows the YASK compiler to bundle all
        // the stress equations together.
        stress_xx(t+1, x, y, z) EQUALS get_next_stress_xx(x, y, z) IF_BELOW_SURFACE;
        stress_yy(t+1, x, y, z) EQUALS get_next_stress_yy(x, y, z) IF_BELOW_SURFACE;
        stress_xy(t+1, x, y, z) EQUALS get_next_stress_xy(x, y, z) IF_BELOW_SURFACE;
        stress_xz(t+1, x, y, z) EQUALS get_next_stress_xz(x, y, z) IF_BELOW_SURFACE;
        stress_yz(t+1, x, y, z) EQUALS get_next_stress_yz(x, y, z) IF_BELOW_SURFACE;
        stress_zz(t+1, x, y, z) EQUALS get_next_stress_zz(x, y, z) IF_BELOW_SURFACE;

        stress_xx(t+1, x, y, z) EQUALS get_next_stress_xx(x, y, z) IF_AT_SURFACE;
        stress_yy(t+1, x, y, z) EQUALS get_next_stress_yy(x, y, z) IF_AT_SURFACE;
        stress_xy(t+1, x, y, z) EQUALS get_next_stress_xy(x, y, z) IF_AT_SURFACE;
        stress_xz(t+1, x, y, z) EQUALS 0.0 IF_AT_SURFACE;
        stress_yz(t+1, x, y, z) EQUALS 0.0 IF_AT_SURFACE;
        stress_zz(t+1, x, y, z) EQUALS get_next_stress_zz(x, y, z) IF_AT_SURFACE;

        // Boundary conditions.
#ifdef DO_ABOVE_SURFACE
        define_free_surface_vel();
        define_free_surface_stress();
#endif
    }
};

REGISTER_STENCIL(AwpElasticStencil);

#undef DO_SURFACE
#undef FULL_SPONGE_GRID
#undef USE_SCRATCH_GRIDS
