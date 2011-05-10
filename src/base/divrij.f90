!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2009 EDF S.A., France

!     contact: saturne-support@edf.fr

!     The Code_Saturne Kernel is free software; you can redistribute it
!     and/or modify it under the terms of the GNU General Public License
!     as published by the Free Software Foundation; either version 2 of
!     the License, or (at your option) any later version.

!     The Code_Saturne Kernel is distributed in the hope that it will be
!     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
!     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!     GNU General Public License for more details.

!     You should have received a copy of the GNU General Public License
!     along with the Code_Saturne Kernel; if not, write to the
!     Free Software Foundation, Inc.,
!     51 Franklin St, Fifth Floor,
!     Boston, MA  02110-1301  USA

!-------------------------------------------------------------------------------

subroutine divrij &
!================

 ( idbia0 , idbra0 ,                                              &
   nvar   , nscal  , nphas  ,                                     &
   idim   , ivar   ,                                              &
   ia     ,                                                       &
   rtpa   , propce , propfa , propfb ,                            &
   coefa  , coefb  ,                                              &
   viscf  , viscb  ,                                              &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , coefu  ,                            &
   ra     )

!===============================================================================
! FONCTION :
! ---------

! DISPOSANT DU TENSEUR Rij
!  ON CALCULE LE TERME EN DIV INTERVENANT DANS L'EQUATION
!    DE LA VITESSE
!  ON PRODUIT DONC SOMME (Rij)kl Skl nkl
!    (Rij)kl EST LA VALEUR A LA FACE kl
!       Skl  EST LA SURFACE DE LA FACE kl
!       nkl  EST LE VECTEUR NORMAL A kl NORME
!       ON SOMME SUR TROIS COMPOSANTES DU TENSEUR
!  ON OBTIENT DONC UNE VALEUR PAR FACE

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! idbia0           ! i  ! <-- ! number of first free position in ia            !
! idbra0           ! i  ! <-- ! number of first free position in ra            !
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! nphas            ! i  ! <-- ! number of phases                               !
! idim             ! e  ! <-- ! composante traitee                             !
! ivar             ! e  ! <-- ! numero de variable courante                    !
! ia(*)            ! ia ! --- ! main integer work array                        !
! rtpa             ! tr ! <-- ! variables de calcul au centre des              !
! (ncelet,*)       !    !     !    cellules (instant prec)                     !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! viscf(nfac)      ! tr ! --> ! resultat du calcul                             !
! viscb(nfabor)    ! tr ! --> ! resultat du calcul                             !
! w1-w9(ncelet)    ! tr ! --- ! tableau de travail                             !
! coefu(nfab,4)    ! tr ! --- ! tableau de travail                             !
! ra(*)            ! ra ! --- ! main real work array                           !
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
use dimens, only: ndimfb
use numvar
use entsor
use cstphy
use optcal
use pointe
use mesh

!===============================================================================

implicit none

! Arguments

integer          idbia0 , idbra0
integer          nvar   , nscal  , nphas
integer          idim   , ivar

integer          ia(*)

double precision rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision viscf(nfac), viscb(nfabor)
double precision w1(ncelet), w2(ncelet), w3(ncelet)
double precision w4(ncelet), w5(ncelet), w6(ncelet)
double precision w7(ncelet), w8(ncelet), w9(ncelet)
double precision coefu(nfabor,3)
double precision ra(*)

! Local variables

integer          idebia, idebra
integer          ifac, ivar1, ivar2, ivar3, init, inc
integer          iccocg,iflmb0
integer          iuiph , iviph , iwiph
integer          ir11ip, ir22ip, ir33ip, ir12ip, ir13ip, ir23ip
integer          ipcrom, ipbrom
integer          iclva1, iclva2, iclva3
integer          nswrgp, imligp, iwarnp
integer          iismph, imaspe
double precision epsrgp, climgp, extrap

!===============================================================================

!===============================================================================
! 1.  INITIALISATION
!===============================================================================

! --- Memoire
idebia = idbia0
idebra = idbra0

! --- Variables
iuiph  = iu
iviph  = iv
iwiph  = iw
ir11ip = ir11
ir22ip = ir22
ir33ip = ir33
ir12ip = ir12
ir13ip = ir13
ir23ip = ir23

! --- Masse volumique
ipcrom = ipproc(irom  )
ipbrom = ipprob(irom  )

! --- Variables locales (Rij)
if(ivar.eq.iuiph) then
   ivar1 = ir11ip
   ivar2 = ir12ip
   ivar3 = ir13ip
elseif(ivar.eq.iviph) then
   ivar1 = ir12ip
   ivar2 = ir22ip
   ivar3 = ir23ip
elseif(ivar.eq.iwiph) then
   ivar1 = ir13ip
   ivar2 = ir23ip
   ivar3 = ir33ip
endif

! --- Conditions aux limites des variables locales (Rij)
iclva1 = iclrtp(ivar1,icoef)
iclva2 = iclrtp(ivar2,icoef)
iclva3 = iclrtp(ivar3,icoef)

!===============================================================================
! 2.  CALCUL DE LA DIVERGENCE
!===============================================================================

! --- Options de calcul
init = 1
inc  = 1
iccocg = 1
iflmb0 = 0
nswrgp = nswrgr(ir11ip)
imligp = imligr(ir11ip)
iwarnp = iwarni(ir11ip)
epsrgp = epsrgr(ir11ip)
climgp = climgr(ir11ip)
extrap = extrag(ir11ip)

iismph = iisymp

imaspe = 2

call inimas                                                       &
!==========
 ( idebia , idebra ,                                              &
   nvar   , nscal  , nphas  ,                                     &
   ivar1  , ivar2  , ivar3  , imaspe ,                            &
   iflmb0 , init   , inc    , imrgra , iccocg , nswrgp , imligp , &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp , extrap ,                                     &
   ia(iismph) ,                                                   &
   ia     ,                                                       &
   propce(1,ipcrom), propfb(1,ipbrom),                            &
   rtpa(1,ivar1)   , rtpa(1,ivar2)   , rtpa(1,ivar3)   ,          &
   coefa(1,iclva1) , coefa(1,iclva2) , coefa(1,iclva3) ,          &
   coefb(1,iclva1) , coefb(1,iclva2) , coefb(1,iclva3) ,          &
   viscf  , viscb  ,                                              &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , coefu  ,                            &
   ra     )


!     Calcul des efforts aux bords (partie 5/5), si necessaire

if (ineedf.eq.1) then
  do ifac = 1, nfabor
    ra(iforbr+(ifac-1)*ndim+idim-1) =                             &
         ra(iforbr+(ifac-1)*ndim+idim-1) + viscb(ifac)
  enddo
endif

return
end subroutine
