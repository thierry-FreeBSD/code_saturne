!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2014 EDF S.A.
!
! This program is free software; you can redistribute it and/or modify it under
! the terms of the GNU General Public License as published by the Free Software
! Foundation; either version 2 of the License, or (at your option) any later
! version.
!
! This program is distributed in the hope that it will be useful, but WITHOUT
! ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
! FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
! details.
!
! You should have received a copy of the GNU General Public License along with
! this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
! Street, Fifth Floor, Boston, MA 02110-1301, USA.

!-------------------------------------------------------------------------------

!> \file numvar.f90
!> \brief Module for variable numbering

module numvar

  !=============================================================================

  use paramx

  implicit none

  !=============================================================================

  !> \defgroup numvar Module for variable numbering

  !> \addtogroup numvar
  !> \{

  !----------------------------------------------------------------------------
  ! Main variables
  !----------------------------------------------------------------------------

  !> \defgroup main_variables Main variables
  !> \brief Main variables stored in rtp, rtpa.

  !> \addtogroup main_variables
  !> \{

  !> pressure
  integer, save :: ipr

  !> velocity component \f$ u_x \f$
  integer, save :: iu

  !> velocity component \f$ u_y \f$
  integer, save :: iv

  !> velocity component \f$ u_z \f$
  integer, save :: iw

  !> turbulent kinetic energy \f$ k \f$
  integer, save :: ik

  !> turbulent dissipation \f$ \varepsilon \f$
  integer, save :: iep

  !> Reynolds stress component \f$ R_{xx} \f$
  integer, save :: ir11

  !> Reynolds stress component \f$ R_{yy} \f$
  integer, save :: ir22

  !> Reynolds stress component \f$ R_{zz} \f$
  integer, save :: ir33

  !> Reynolds stress component \f$ R_{xy} \f$
  integer, save :: ir12

  !> Reynolds stress component \f$ R_{yz} \f$
  integer, save :: ir23

  !> Reynolds stress component \f$ R_{zz} \f$
  integer, save :: ir13

  !> variable \f$ \phi \f$ of the \f$ \phi-f_b \f$ model
  integer, save :: iphi

  !> variable \f$ f_b \f$ of the \f$ \phi-f_b \f$ model
  integer, save :: ifb

  !> variable \f$ \alpha \f$ of the \f$ Bl-v^2-k \f$ model
  integer, save :: ial

  !> variable \f$ \omega \f$ of the \f$ k-\omega \f$ SST
  integer, save :: iomg

  !> variable \f$ \widetilde{\nu}_T \f$ of the Spalart Allmaras
  integer, save :: inusa

  !> isca(i) is the index of the scalar i
  integer, save :: isca(nscamx)

  !> iscapp(i) is the index of the specific physics scalar i
  integer, save :: iscapp(nscamx)

  !> number of user scalars
  integer, save :: nscaus

  !> number of specific physics scalars
  integer, save :: nscapp

  !> number of species scalars
  integer, save :: nscasp

  !> mesh velocity component \f$ w_x \f$
  integer, save :: iuma

  !> mesh velocity component \f$ w_y \f$
  integer, save :: ivma

  !> mesh velocity component \f$ w_z \f$
  integer, save :: iwma

  !> \}

  !----------------------------------------------------------------------------
  ! Physical properties
  !----------------------------------------------------------------------------

  !> \defgroup physical_prop Physical properties
  !> \brief Physical properties are stored in propce.
  !> See \ref cs_user_boundary_conditions for some examples.

  !> \addtogroup physical_prop
  !> \{

  !> pointer to cell properties (propce)
  integer, save :: ipproc(npromx)

  !> Density at the current time step
  integer, save :: irom

  !> Density at the previous time step
  integer, save :: iroma

  !> dynamic molecular viscosity (in kg/(m.s))
  integer, save :: iviscl

  !> dynamic turbulent viscosity
  integer, save :: ivisct

  !> dynamic molecular viscosity (in kg/(m.s)) at the previous time-step
  integer, save :: ivisla

  !> dynamic turbulent viscosity at the previous time-step
  integer, save :: ivista

  !> specific heat \f$ C_p \f$
  integer, save :: icp

  !> specific heat \f$ C_p \f$ at the previous time-step
  integer, save :: icpa

  !> Navier-Stokes source terms at the previous time-step
  integer, save :: itsnsa

  !> Turbulent source terms at the previous time-step
  integer, save :: itstua

  !> transported scalars source terms at the previous time-step
  integer, save :: itssca(nscamx)

  !> error estimator for Navier-Stokes
  integer, save :: iestim(nestmx)

  !> interior and boundary convective mass flux key ids of the variables
  integer, save :: kimasf, kbmasf

  !> convective mass flux of the variables at the previous time-step
  integer, save :: ifluaa(nvarmx)

  !> cell and boundary density field ids of the variables
  integer, save :: icrom, ibrom

  !> cell porosity key ids of the properties
  integer, save :: ipori, iporf

  !> dynamic constant of Smagorinsky
  integer, save :: ismago

  !> Courant number
  integer, save :: icour

  !> Fourier number
  integer, save :: ifour

  !> Total pressure at cell centres
  !> \f$ P_{tot} = P^\star +\rho \vect{g} \cdot (\vect{x}-\vect{x}_0) \f$
  integer, save :: iprtot

  !> Mesh velocity viscosity for the ALE module
  !> \remark might be orthotropic
  integer, save :: ivisma(3)

  !> pointer for dilatation source terms
  integer, save :: iustdy(nscamx)

  !> pointer for global dilatation source terms
  integer, save :: itsrho

  !> pointer for thermal expansion coefficient
  integer, save :: ibeta

  !> \}

  !----------------------------------------------------------------------------
  ! Mapping to field structures
  !----------------------------------------------------------------------------

  !> \defgroup field_map Mapping to field structures

  !> \addtogroup field_map
  !> \{

  !> Field id for variable i
  integer, save :: ivarfl(nvarmx)

  !> Field id for property i
  integer, save :: iprpfl(npromx)

  !> Field id for the dttens tensor
  integer, save :: idtten

  !> \}

  !=============================================================================

  !> \}

end module numvar
