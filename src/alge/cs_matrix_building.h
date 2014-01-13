#ifndef __CS_MATRIX_BUILDING_H__
#define __CS_MATRIX_BUILDING_H__

/*============================================================================
 * Matrix building
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2014 EDF S.A.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_halo.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definition
 *============================================================================*/

/*============================================================================
 *  Global variables
 *============================================================================*/

/*============================================================================
 * Public function prototypes for Fortran API
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Wrapper to cs_matrix
 *----------------------------------------------------------------------------*/

void CS_PROCF (matrix, MATRIX)
(
 const cs_int_t  *const   n_cells_ext,
 const cs_int_t  *const   n_cells,
 const cs_int_t  *const   n_i_faces,
 const cs_int_t  *const   n_b_faces,
 const cs_int_t  *const   iconvp,
 const cs_int_t  *const   idiffp,
 const cs_int_t  *const   ndircp,
 const cs_int_t  *const   isym,
 const cs_int_t  *const   nfecra,
 const cs_real_t *const   thetap,
 const cs_int_t  *const   imucpp,
 const cs_lnum_2_t        i_face_cells[],
 const cs_int_t           b_face_cells[],
 const cs_real_t          coefbp[],
 const cs_real_t          cofbfp[],
 const cs_real_t          rovsdt[],
 const cs_real_t          i_massflux[],
 const cs_real_t          b_massflux[],
 const cs_real_t          i_visc[],
 const cs_real_t          b_visc[],
 const cs_real_t          xcpp[],
 cs_real_t                da[],
 cs_real_t                xa[][*n_i_faces]);

/*----------------------------------------------------------------------------
 * Wrapper to cs_matrxv
 *----------------------------------------------------------------------------*/

void CS_PROCF (matrxv, MATRXV)
(
 const cs_int_t  *const   n_cells_ext,
 const cs_int_t  *const   n_cells,
 const cs_int_t  *const   n_i_faces,
 const cs_int_t  *const   n_b_faces,
 const cs_int_t  *const   iconvp,
 const cs_int_t  *const   idiffp,
 const cs_int_t  *const   ndircp,
 const cs_int_t  *const   isym,
 const cs_int_t  *const   nfecra,
 const cs_real_t *const   thetap,
 const cs_lnum_2_t        i_face_cells[],
 const cs_int_t           b_face_cells[],
 const cs_real_33_t       coefbu[],
 const cs_real_33_t       cofbfu[],
 const cs_real_33_t       fimp[],
 const cs_real_t          i_massflux[],
 const cs_real_t          b_massflux[],
 const cs_real_t          i_visc[],
 const cs_real_t          b_visc[],
 cs_real_33_t             da[],
 cs_real_2_t              xa[]);

/*----------------------------------------------------------------------------
 * Wrapper to cs_matrdr
 *----------------------------------------------------------------------------*/

void CS_PROCF (matrdt, MATRDT)
(
 const cs_int_t  *const   iconvp,
 const cs_int_t  *const   idiffp,
 const cs_int_t  *const   isym,
 const cs_real_t          coefbp[],
 const cs_real_t          cofbfp[],
 const cs_real_t          i_massflux[],
 const cs_real_t          b_massflux[],
 const cs_real_t          i_visc[],
 const cs_real_t          b_visc[],
 cs_real_t                da[]);

/*----------------------------------------------------------------------------
 * Wrapper to cs_matrvv
 *----------------------------------------------------------------------------*/

void CS_PROCF (matrvv, MATRVV)
(
 const cs_int_t  *const   n_cells_ext,
 const cs_int_t  *const   n_cells,
 const cs_int_t  *const   n_i_faces,
 const cs_int_t  *const   n_b_faces,
 const cs_int_t  *const   iconvp,
 const cs_int_t  *const   idiffp,
 const cs_int_t  *const   ndircp,
 const cs_int_t  *const   isym,
 const cs_int_t  *const   nfecra,
 const cs_real_t *const   thetap,
 const cs_lnum_2_t        i_face_cells[],
 const cs_int_t           b_face_cells[],
 const cs_real_33_t       coefbu[],
 const cs_real_33_t       cofbfu[],
 const cs_real_33_t       fimp[],
 const cs_real_t          i_massflux[],
 const cs_real_t          b_massflux[],
 const cs_real_33_t       i_visc[],
 const cs_real_t          b_visc[],
 cs_real_33_t             da[],
 cs_real_332_t            xa[]);

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*! \brief This function builds the matrix of advection/diffusion for a scalar
  field.

  The advection is upwind, the diffusion is not reconstructed.
  The matrix is splitted into a diagonal block (number of cells)
  and an extra diagonal part (of dimension 2 time the number of internal
  faces).

*/
/*------------------------------------------------------------------------------
  Arguments
 ______________________________________________________________________________.
   mode           name          role                                           !
 _____________________________________________________________________________*/
/*!
 * \param[in]     n_cells_ext   number of extended (real + ghost) cells
 * \param[in]     n_cells       number of cells
 * \param[in]     n_i_faces     number of interior faces
 * \param[in]     n_b_faces     number of boundary faces
 * \param[in]     iconvp        indicator
 *                               - 1 advection
 *                               - 0 otherwise
 * \param[in]     idiffp        indicator
 *                               - 1 diffusion
 *                               - 0 otherwise
 * \param[in]     ndircp        indicator
 *                               - 0 if the diagonal stepped aside
 * \param[in]     isym          indicator
 *                               - 1 symmetric matrix
 *                               - 2 non symmmetric matrix
 * \param[in]     thetap        weightening coefficient for the theta-schema,
 *                               - thetap = 0: explicit scheme
 *                               - thetap = 0.5: time-centred
 *                               scheme (mix between Crank-Nicolson and
 *                               Adams-Bashforth)
 *                               - thetap = 1: implicit scheme
 * \param[in]     imucpp        indicator
 *                               - 0 do not multiply the convectiv term by Cp
 *                               - 1 do multiply the convectiv term by Cp
 * \param[in]     i_face_cells  cell indices of interior faces
 * \param[in]     b_face_cells  cell indices of boundary faces
 * \param[in]     coefbp        boundary condition array for the variable
 *                               (Impplicit part)
 * \param[in]     cofbfp        boundary condition array for the variable flux
 *                               (Impplicit part)
 * \param[in]     rovsdt        working array
 * \param[in]     i_massflux    mass flux at interior faces
 * \param[in]     b_massflux    mass flux at border faces
 * \param[in]     i_visc        \f$ \mu_\fij \dfrac{S_\fij}{\ipf \jpf} \f$
 *                               at interior faces for the matrix
 * \param[in]     b_visc        \f$ S_\fib \f$
 *                               at border faces for the matrix
 * \param[in]     xcpp          array of specific heat (Cp)
 * \param[out]    da            diagonal part of the matrix
 * \param[out]    xa            extra interleaved diagonal part of the matrix
 */
/*----------------------------------------------------------------------------*/

void
cs_matrix_scalar(
                 int                       n_cells_ext,
                 int                       n_cells,
                 int                       n_i_faces,
                 int                       n_b_faces,
                 int                       iconvp,
                 int                       idiffp,
                 int                       ndircp,
                 int                       isym,
                 double                    thetap,
                 int                       imucpp,
                 const cs_lnum_2_t         i_face_cells[],
                 const cs_int_t            b_face_cells[],
                 const cs_real_t           coefbp[],
                 const cs_real_t           cofbfp[],
                 const cs_real_t           rovsdt[],
                 const cs_real_t           i_massflux[],
                 const cs_real_t           b_massflux[],
                 const cs_real_t           i_visc[],
                 const cs_real_t           b_visc[],
                 const cs_real_t           xcpp[],
                 cs_real_t       *restrict da,
                 cs_real_t                 xa[][n_i_faces]);

/*----------------------------------------------------------------------------*/

/*! \brief This function builds the matrix of advection/diffusion for a vector
  field.

  The advection is upwind, the diffusion is not reconstructed.
  The matrix is splitted into a diagonal block (3x3 times number of cells)
  and an extra diagonal part (of dimension 2 time the number of internal
  faces).

*/
/*------------------------------------------------------------------------------
  Arguments
 ______________________________________________________________________________.
   mode           name          role                                           !
 _____________________________________________________________________________*/
/*!
 * \param[in]     n_cells_ext   number of extended (real + ghost) cells
 * \param[in]     n_cells       number of cells
 * \param[in]     n_i_faces     number of interior faces
 * \param[in]     n_b_faces     number of boundary faces
 * \param[in]     iconvp        indicator
 *                               - 1 advection
 *                               - 0 otherwise
 * \param[in]     idiffp        indicator
 *                               - 1 diffusion
 *                               - 0 otherwise
 * \param[in]     ndircp        indicator
 *                               - 0 if the diagonal stepped aside
 * \param[in]     isym          indicator
 *                               - 1 symmetric matrix
 *                               - 2 non symmmetric matrix
 * \param[in]     thetap        weightening coefficient for the theta-schema,
 *                               - thetap = 0: explicit scheme
 *                               - thetap = 0.5: time-centred
 *                               scheme (mix between Crank-Nicolson and
 *                               Adams-Bashforth)
 *                               - thetap = 1: implicit scheme
 * \param[in]     i_face_cells  cell indices of interior faces
 * \param[in]     b_face_cells  cell indices of boundary faces
 * \param[in]     coefbu        boundary condition array for the variable
 *                               (Impplicit part - 3x3 tensor array)
 * \param[in]     cofbfu        boundary condition array for the variable flux
 *                               (Impplicit part - 3x3 tensor array)
 * \param[in]     fimp          \f$ \tens{f_s}^{imp} \f$
 * \param[in]     i_massflux    mass flux at interior faces
 * \param[in]     b_massflux    mass flux at border faces
 * \param[in]     i_visc        \f$ \mu_\fij \dfrac{S_\fij}{\ipf \jpf} \f$
 *                               at interior faces for the matrix
 * \param[in]     b_visc        \f$ \mu_\fib \dfrac{S_\fib}{\ipf \centf} \f$
 *                               at border faces for the matrix
 * \param[out]    da            diagonal part of the matrix
 * \param[out]    xa            extra interleaved diagonal part of the matrix
 */
/*----------------------------------------------------------------------------*/

void
cs_matrix_vector(
                 int                       n_cells_ext,
                 int                       n_cells,
                 int                       n_i_faces,
                 int                       n_b_faces,
                 int                       iconvp,
                 int                       idiffp,
                 int                       ndircp,
                 int                       isym,
                 double                    thetap,
                 const cs_lnum_2_t         i_face_cells[],
                 const cs_int_t            b_face_cells[],
                 const cs_real_33_t        coefbu[],
                 const cs_real_33_t        cofbfu[],
                 const cs_real_33_t        fimp[],
                 const cs_real_t           i_massflux[],
                 const cs_real_t           b_massflux[],
                 const cs_real_t           i_visc[],
                 const cs_real_t           b_visc[],
                 cs_real_33_t    *restrict da,
                 cs_real_2_t     *restrict xa);

/*----------------------------------------------------------------------------*/

/*! \brief Construction of the diagonal of the advection/diffusion matrix
  for determining the variable time step, flow, Fourier.

*/
/*------------------------------------------------------------------------------
  Arguments
 ______________________________________________________________________________.
   mode           name          role                                           !
 _____________________________________________________________________________*/
/*!
 * \param[in]     iconvp        indicator
 *                               - 1 advection
 *                               - 0 otherwise
 * \param[in]     idiffp        indicator
 *                               - 1 diffusion
 *                               - 0 otherwise
 * \param[in]     isym          indicator
 *                               - 1 symmetric matrix
 *                               - 2 non symmmetric matrix
 * \param[in]     coefbp        boundary condition array for the variable
 *                               (Impplicit part)
 * \param[in]     cofbfp        boundary condition array for the variable flux
 *                               (Impplicit part)
 * \param[in]     i_massflux    mass flux at interior faces
 * \param[in]     b_massflux    mass flux at border faces
 * \param[in]     i_visc        \f$ \mu_\fij \dfrac{S_\fij}{\ipf \jpf} \f$
 *                               at interior faces for the matrix
 * \param[in]     b_visc        \f$ S_\fib \f$
 *                               at border faces for the matrix
 * \param[out]    da            diagonal part of the matrix
 */
/*----------------------------------------------------------------------------*/

void
cs_matrix_time_step(
                    int                       iconvp,
                    int                       idiffp,
                    int                       isym,
                    const cs_real_t           coefbp[],
                    const cs_real_t           cofbfp[],
                    const cs_real_t           i_massflux[],
                    const cs_real_t           b_massflux[],
                    const cs_real_t           i_visc[],
                    const cs_real_t           b_visc[],
                    cs_real_t       *restrict da);

/*----------------------------------------------------------------------------*/

/*! \brief This function builds the matrix of advection/diffusion for a vector
  field with a tensorial diffusivity.

  The advection is upwind, the diffusion is not reconstructed.
  The matrix is splitted into a diagonal block (3x3 times number of cells)
  and an extra diagonal part (of dimension 2 times 3x3 the number of internal
  faces).

*/
/*------------------------------------------------------------------------------
  Arguments
 ______________________________________________________________________________.
   mode           name          role                                           !
 _____________________________________________________________________________*/
/*!
 * \param[in]     n_cells_ext   number of extended (real + ghost) cells
 * \param[in]     n_cells       number of cells
 * \param[in]     n_i_faces     number of interior faces
 * \param[in]     n_b_faces     number of boundary faces
 * \param[in]     iconvp        indicator
 *                               - 1 advection
 *                               - 0 otherwise
 * \param[in]     idiffp        indicator
 *                               - 1 diffusion
 *                               - 0 otherwise
 * \param[in]     ndircp        indicator
 *                               - 0 if the diagonal stepped aside
 * \param[in]     isym          indicator
 *                               - 1 symmetric matrix
 *                               - 2 non symmmetric matrix
 * \param[in]     thetap        weightening coefficient for the theta-schema,
 *                               - thetap = 0: explicit scheme
 *                               - thetap = 0.5: time-centred
 *                               scheme (mix between Crank-Nicolson and
 *                               Adams-Bashforth)
 *                               - thetap = 1: implicit scheme
 * \param[in]     i_face_cells  cell indices of interior faces
 * \param[in]     b_face_cells  cell indices of boundary faces
 * \param[in]     coefbu        boundary condition array for the variable
 *                               (Impplicit part - 3x3 tensor array)
 * \param[in]     cofbfu        boundary condition array for the variable flux
 *                               (Impplicit part - 3x3 tensor array)
 * \param[in]     fimp
 * \param[in]     i_massflux    mass flux at interior faces
 * \param[in]     b_massflux    mass flux at border faces
 * \param[in]     i_visc        \f$ \mu_\fij \dfrac{S_\fij}{\ipf \jpf} \f$
 *                               at interior faces for the matrix
 * \param[in]     b_visc        \f$ \mu_\fib \dfrac{S_\fib}{\ipf \centf} \f$
 *                               at border faces for the matrix
 * \param[out]    da            diagonal part of the matrix
 * \param[out]    xa            extra interleaved diagonal part of the matrix
 */
/*----------------------------------------------------------------------------*/

void
cs_matrix_tensorial_diffusion(
                              int                       n_cells_ext,
                              int                       n_cells,
                              int                       n_i_faces,
                              int                       n_b_faces,
                              int                       iconvp,
                              int                       idiffp,
                              int                       ndircp,
                              int                       isym,
                              double                    thetap,
                              const cs_lnum_2_t         i_face_cells[],
                              const cs_int_t            b_face_cells[],
                              const cs_real_33_t        coefbu[],
                              const cs_real_33_t        cofbfu[],
                              const cs_real_33_t        fimp[],
                              const cs_real_t           i_massflux[],
                              const cs_real_t           b_massflux[],
                              const cs_real_33_t        i_visc[],
                              const cs_real_t           b_visc[],
                              cs_real_33_t    *restrict da,
                              cs_real_332_t   *restrict xa);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_MATRIX_BUILDING_H__ */
