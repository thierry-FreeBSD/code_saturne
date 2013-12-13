!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2013 EDF S.A.
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

subroutine cfdivs &
!================

 ( rtpa   , propce ,                                              &
   diverg , ux     , uy     , uz     )

!===============================================================================
! FONCTION :
! ----------
!                                   v
! CALCULE DIVERG = DIVERG + DIV(SIGMA .U)

!          v               t
! AVEC SIGMA = MU (GRAD(U) + GRAD(U)) + (KAPPA - 2/3 MU) DIV(U) Id

! ET MU = MU_LAMINAIRE + MU_TURBULENT

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! rtpa             ! tr ! <-- ! variables de calcul au centre des              !
! (ncelet,*)       !    !     !    cellules (instant precedent)                !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! diverg(ncelet    ! tr ! --> ! div(sigma.u)                                   !
! ux,y,z(ncelet    ! tr ! <-- ! composantes du vecteur u                       !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail

!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use cstphy
use entsor
use numvar
use optcal
use parall
use period
use ppppar
use ppthch
use ppincl
use mesh
use field

!===============================================================================

implicit none

! Arguments

double precision rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision diverg(ncelet)
double precision ux(ncelet), uy(ncelet), uz(ncelet)

! Local variables

integer          inc, iel, ifac, ii, jj
integer          nswrgp, imligp, iwarnp
integer          ipcvis, ipcvst, ipcvsv

logical          ilved

double precision epsrgp, climgp, extrap
double precision vecfac, kappa, mu, trgdru
double precision sigma(3,3)

double precision, allocatable, dimension(:) :: vistot
double precision, allocatable, dimension(:,:,:) :: gradv
double precision, allocatable, dimension(:,:) :: tempv
double precision, dimension(:,:), pointer :: coefau
double precision, dimension(:,:,:), pointer :: coefbu

!===============================================================================

!===============================================================================
! 1. Initialization
!===============================================================================

call field_get_coefa_v(ivarfl(iu), coefau)
call field_get_coefb_v(ivarfl(iu), coefbu)

! Allocate temporary arrays
allocate(vistot(ncelet))
allocate(gradv(3,3,ncelet))
allocate(tempv(3, ncelet))

ipcvis = ipproc(iviscl)
ipcvst = ipproc(ivisct)
if(iviscv.gt.0) then
  ipcvsv = ipproc(iviscv)
else
  ipcvsv = 0
endif

! --- Calcul de la viscosite totale

if (itytur.eq.3 ) then
  do iel = 1, ncel
    vistot(iel) = propce(iel,ipcvis)
  enddo
else
  do iel = 1, ncel
    vistot(iel) = propce(iel,ipcvis) + propce(iel,ipcvst)
  enddo
endif

! ---> Periodicity and parallelism treatment

if (irangp.ge.0.or.iperio.eq.1) then
  call synsca(vistot)
  if (ipcvsv.gt.0) then
    call synsca(propce(1,ipcvsv))
  endif
endif

!===============================================================================
! 2. Compute the divegence of (sigma.u)
!===============================================================================

! --- Velocity gradient
inc    = 1
nswrgp = nswrgr(iu)
imligp = imligr(iu)
iwarnp = iwarni(iu)
epsrgp = epsrgr(iu)
climgp = climgr(iu)
extrap = extrag(iu)

ilved = .false.

! WARNING: gradv(xyz, uvw, iel)
call grdvec &
!==========
( iu     , imrgra , inc    , nswrgp , imligp ,                   &
  iwarnp , epsrgp , climgp ,                                     &
  ilved  ,                                                       &
  rtpa(1,iu) ,  coefau , coefbu ,                                &
  gradv  )

! --- Compute the vector \tens{\sigma}.\vect{v}
!     i.e. sigma_ij v_j e_i

! Variable kappa in space
if (ipcvsv.gt.0) then
  do iel = 1, ncel
    kappa = propce(iel,ipcvsv)
    mu = vistot(iel)
    trgdru = gradv(1, 1, iel)+gradv(2, 2, iel)+gradv(3, 3, iel)

    sigma(1, 1) = mu * 2.d0*gradv(1, 1, iel)  &
                + (kappa-2.d0/3.d0*mu)*trgdru

    sigma(2, 2) = mu * 2.d0*gradv(2, 2, iel)  &
                + (kappa-2.d0/3.d0*mu)*trgdru

    sigma(3, 3) = mu * 2.d0*gradv(3, 3, iel)  &
                + (kappa-2.d0/3.d0*mu)*trgdru

    sigma(1, 2) = mu * (gradv(1, 2, iel) + gradv(2, 1, iel))
    sigma(2, 1) = sigma(1, 2)

    sigma(2, 3) = mu * (gradv(2, 3, iel) + gradv(3, 2, iel))
    sigma(3, 2) = sigma(2, 3)

    sigma(1, 3) = mu * (gradv(1, 3, iel) + gradv(3, 1, iel))
    sigma(3, 1) = sigma(1, 3)

    tempv(1, iel) = sigma(1, 1)*ux(iel) &
                  + sigma(1, 2)*uy(iel) &
                  + sigma(1, 3)*uz(iel)
    tempv(2, iel) = sigma(2, 1)*ux(iel) &
                  + sigma(2, 2)*uy(iel) &
                  + sigma(2, 3)*uz(iel)
    tempv(3, iel) = sigma(3, 1)*ux(iel) &
                  + sigma(3, 2)*uy(iel) &
                  + sigma(3, 3)*uz(iel)
  enddo

else

  do iel = 1, ncel
    kappa = viscv0
    mu = vistot(iel)
    trgdru = gradv(1, 1, iel)+gradv(2, 2, iel)+gradv(3, 3, iel)

    sigma(1, 1) = mu * 2.d0*gradv(1, 1, iel)  &
                + (kappa-2.d0/3.d0*mu)*trgdru

    sigma(2, 2) = mu * 2.d0*gradv(2, 2, iel)  &
                + (kappa-2.d0/3.d0*mu)*trgdru

    sigma(3, 3) = mu * 2.d0*gradv(3, 3, iel)  &
                + (kappa-2.d0/3.d0*mu)*trgdru

    sigma(1, 2) = mu * (gradv(1, 2, iel) + gradv(2, 1, iel))
    sigma(2, 1) = sigma(1, 2)

    sigma(2, 3) = mu * (gradv(2, 3, iel) + gradv(3, 2, iel))
    sigma(3, 2) = sigma(2, 3)

    sigma(1, 3) = mu * (gradv(1, 3, iel) + gradv(3, 1, iel))
    sigma(3, 1) = sigma(1, 3)

    tempv(1, iel) = sigma(1, 1)*ux(iel) &
                  + sigma(1, 2)*uy(iel) &
                  + sigma(1, 3)*uz(iel)
    tempv(2, iel) = sigma(2, 1)*ux(iel) &
                  + sigma(2, 2)*uy(iel) &
                  + sigma(2, 3)*uz(iel)
    tempv(3, iel) = sigma(3, 1)*ux(iel) &
                  + sigma(3, 2)*uy(iel) &
                  + sigma(3, 3)*uz(iel)
  enddo

endif

! ---> Periodicity and parallelism treatment

if (irangp.ge.0.or.iperio.eq.1) then
  call synvin(tempv)
endif

! Initialize diverg(ncel+1, ncelet)
!  (unused value, but need to be initialized to avoid Nan values)
if (ncelet.gt.ncel) then
  do iel = ncel+1, ncelet
    diverg(iel) = 0.d0
  enddo
endif

! --- Interior faces contribution

do ifac = 1, nfac
  ii = ifacel(1,ifac)
  jj = ifacel(2,ifac)
  vecfac = surfac(1,ifac)*(tempv(1, ii)+tempv(1, jj))*0.5d0               &
         + surfac(2,ifac)*(tempv(2, ii)+tempv(2, jj))*0.5d0               &
         + surfac(3,ifac)*(tempv(3, ii)+tempv(3, jj))*0.5d0
  diverg(ii) = diverg(ii) + vecfac
  diverg(jj) = diverg(jj) - vecfac
enddo

! --- Boundary faces contribution

do ifac = 1, nfabor
  ii = ifabor(ifac)
  vecfac = surfbo(1,ifac)*tempv(1, ii)                                    &
         + surfbo(2,ifac)*tempv(2, ii)                                    &
         + surfbo(3,ifac)*tempv(3, ii)
  diverg(ii) = diverg(ii) + vecfac
enddo

return

end subroutine
