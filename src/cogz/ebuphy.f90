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

subroutine ebuphy &
!================

 ( idbia0 , idbra0 ,                                              &
   nvar   , nscal  , nphas  ,                                     &
   nphmx  ,                                                       &
   ibrom  , izfppp ,                                              &
   ia     ,                                                       &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  ,                                              &
   yfuegf , yoxygf , yprogf ,                                     &
   yfuegb , yoxygb , yprogb ,                                     &
   temp   , masmel ,                                              &
   ra     )

!===============================================================================
! FONCTION :
! --------

! ROUTINE PHYSIQUE PARTICULIERE : FLAMME DE PREMELAMGE MODELE EBU
! Calcul de RHO adiabatique ou permeatique (transport de H)


! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! idbia0           ! i  ! <-- ! number of first free position in ia            !
! idbra0           ! i  ! <-- ! number of first free position in ra            !
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! nphas            ! i  ! <-- ! number of phases                               !
! nphmx            ! e  ! <-- ! nphsmx                                         !
! ibrom            ! te ! <-- ! indicateur de remplissage de romb              !
!        !    !     !                                                !
! izfppp           ! te ! <-- ! numero de zone de la face de bord              !
! (nfabor)         !    !     !  pour le module phys. part.                    !
! ia(*)            ! ia ! --- ! main integer work array                        !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! w1...8(ncelet    ! tr ! --- ! tableau de travail                             !
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
use numvar
use optcal
use cstphy
use cstnum
use entsor
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use radiat
use mesh

!===============================================================================

implicit none

! Arguments

integer          idbia0 , idbra0
integer          nvar   , nscal  , nphas
integer          nphmx

integer          ibrom
integer          izfppp(nfabor)
integer          ia(*)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,*), coefb(nfabor,*)
double precision yfuegf(ncelet),yoxygf(ncelet),yprogf(ncelet)
double precision yfuegb(ncelet),yoxygb(ncelet),yprogb(ncelet)
double precision temp(ncelet),masmel(ncelet)
double precision ra(*)

! Local variables

integer          idebia, idebra
integer          ipctem, ipcfue, ipcoxy, ipcpro
integer          igg, iel, ipcrom
integer          ipckab, ipt4, ipt3
integer          ifac, izone
integer          ipbrom, ipbycg, ipcycg, mode
double precision coefg(ngazgm), ygfm, ygbm, epsi
double precision nbmol , temsmm , fmel , ckabgf, ckabgb
double precision masmgb, hgb, tgb, masmgf, masmg

integer       ipass
data          ipass /0/
save          ipass

!===============================================================================

!===============================================================================
! 0. ON COMPTE LES PASSAGES
!===============================================================================

ipass = ipass + 1

!===============================================================================
! 1. INITIALISATIONS A CONSERVER
!===============================================================================

! Initialize variables to avoid compiler warnings

ipckab = 0
ipt3 = 0
ipt4 = 0

ckabgb = 0.d0
ckabgf = 0.d0

! --- Initialisation memoire

idebia = idbia0
idebra = idbra0

! ---> Initialisation

do igg = 1, ngazgm
  coefg(igg) = zero
enddo

! ---> Positions des variables, coefficients

ipcrom = ipproc(irom)
ipbrom = ipprob(irom)
ipctem = ipproc(itemp)
ipcfue = ipproc(iym(1))
ipcoxy = ipproc(iym(2))
ipcpro = ipproc(iym(3))
if ( iirayo.gt.0 ) then
  ipckab = ipproc(ickabs)
  ipt4 = ipproc(it4m)
  ipt3 = ipproc(it3m)
endif


!===============================================================================
! 2. DETERMINATION DES GRANDEURS THERMOCHIMIQUES
!===============================================================================

! ---> Grandeurs GAZ FRAIS

! ----- Fournies par l'utilisateur
!       FMEL         --> Taux de melange
!                        constant pour les options 0 et 1
!                        variable sinon
!       TGF          --> Temperature gaz frais en K identique
!                        pour premelange frais et dilution
! ----- Deduites
!       YFUEGF(    .)    --> Fraction massique fuel gaz frais
!       YOXYGF(    .)    --> Fraction massique oxydant gaz frais
!       YPROGF(    .)    --> Fraction massique produits gaz frais
!       HGF          --> Enthalpie massique gaz frais identique
!                        pour premelange frais et dilution
!       MASMGF       --> Masse molaire gaz frais
!       CKABGF       --> Coefficient d'absorption

! ---> Grandeurs GAZ BRULES

! ----- Deduites
!       TGB          --> Temperature gaz brules en K
!       YFUEGB(    .)    --> Fraction massique fuel gaz brules
!       YOXYGB(    .)    --> Fraction massique oxydant gaz brules
!       YPROGB(    .)    --> Fraction massique produits gaz brules
!       MASMGB       --> Masse molaire gaz brules
!       CKABGB       --> Coefficient d'absorption

! ---> Grandeurs MELANGE

!       MASMEL           --> Masse molaire du melange
!       PROPCE(    .,IPCTEM) --> Temperature du melange
!       PROPCE(    .,IPCROM) --> Masse volumique du melange
!       PROPCE(,.F,O,P ) --> Fractions massiques en F, O, P
!       PROPCE(    .,IPCKAB) --> Coefficient d'absorption
!       PROPCE(    .,IPT4  ) --> terme T^4
!       PROPCE(    .,IPT3  ) --> terme T^3


! ---> Fractions massiques des gaz frais et brules en F, O, P

do iel = 1, ncel

  if ( ippmod(icoebu).eq.0 .or. ippmod(icoebu).eq.1 ) then
    fmel = frmel
  else
    fmel = rtp(iel,isca(ifm))
  endif

  yfuegf(iel) = fmel
  yoxygf(iel) = 1.d0-fmel
  yprogf(iel) = 0.d0

  yfuegb(iel) = max(zero,(fmel-fs(1))/(1.d0-fs(1)))
  yprogb(iel) = (fmel-yfuegb(iel))/fs(1)
  yoxygb(iel) = 1.d0 - yfuegb(iel) - yprogb(iel)

enddo

epsi = 1.d-06

do iel = 1, ncel

! ---> Coefficients d'absorption des gaz frais et brules

  if ( iirayo.gt.0 ) then
     ckabgf = yfuegf(iel)*ckabsg(1) + yoxygf(iel)*ckabsg(2)       &
            + yprogf(iel)*ckabsg(3)
     ckabgb = yfuegb(iel)*ckabsg(1) + yoxygb(iel)*ckabsg(2)       &
            + yprogb(iel)*ckabsg(3)
  endif

! ---> Masse molaire des gaz frais

  coefg(1) = yfuegf(iel)
  coefg(2) = yoxygf(iel)
  coefg(3) = yprogf(iel)
  nbmol = 0.d0
  do igg = 1, ngazg
    nbmol = nbmol + coefg(igg)/wmolg(igg)
  enddo
  masmgf = 1.d0/nbmol

! ---> Calcul de l'enthalpie des gaz frais

  mode    = -1
  call cothht                                                     &
  !==========
  ( mode   , ngazg , ngazgm  , coefg  ,                           &
    npo    , npot   , th     , ehgazg ,                           &
    hgf    , tgf    )

! ---> Masse molaire des gaz brules

  coefg(1) = yfuegb(iel)
  coefg(2) = yoxygb(iel)
  coefg(3) = yprogb(iel)
  nbmol = 0.d0
  do igg = 1, ngazg
    nbmol = nbmol + coefg(igg)/wmolg(igg)
  enddo
  masmgb = 1.d0/nbmol

  ygfm = rtp(iel,isca(iygfm))
  ygbm = 1.d0 - ygfm

! ---> Masse molaire du melange

  masmel(iel) = 1.d0 / ( ygfm/masmgf + ygbm/masmgb )

! ---> Calcul Temperature des gaz brules

  if ( ippmod(icoebu).eq.0 .or. ippmod(icoebu).eq.2 ) then
! ---- EBU Standard et modifie en conditions adiabatiques (sans H)
    hgb = hgf
  else if ( ippmod(icoebu).eq.1 .or. ippmod(icoebu).eq.3 ) then
! ---- EBU Standard et modifie en conditions permeatiques (avec H)
    hgb = hgf
    if ( ygbm.gt.epsi ) then
      hgb = ( rtp(iel,isca(ihm))-hgf*ygfm ) / ygbm
    endif
  endif

  mode = 1
  call cothht                                                     &
  !==========
  ( mode   , ngazg , ngazgm  , coefg  ,                           &
    npo    , npot   , th     , ehgazg ,                           &
    hgb    , tgb    )

  if ( ippmod(icoebu).eq.0 .or. ippmod(icoebu).eq.2 ) then
! ---- EBU Standard et modifie en conditions adiabatiques (sans H)
    tgbad = tgb
  endif

! ---> Temperature du melange
!      Rq PPl : Il serait plus judicieux de ponderer par les CP (GF et GB)
  propce(iel,ipctem) = ygfm*tgf + ygbm*tgb

! ---> Temperature / Masse molaire

  temsmm = ygfm*tgf/masmgf + ygbm*tgb/masmgb

! ---> Masse volumique du melange

  if (ipass.gt.1.or.(isuite.eq.1.and.initro.eq.1)) then
    propce(iel,ipcrom) = srrom*propce(iel,ipcrom)                 &
                       + (1.d0-srrom)*                            &
                         ( p0/(rr*temsmm) )
  endif

! ---> Fractions massiques des especes globales

  propce(iel,ipcfue) = yfuegf(iel)*ygfm + yfuegb(iel)*ygbm
  propce(iel,ipcoxy) = yoxygf(iel)*ygfm + yoxygb(iel)*ygbm
  propce(iel,ipcpro) = yprogf(iel)*ygfm + yprogb(iel)*ygbm

! ---> Grandeurs relatives au rayonnement

  if ( iirayo.gt.0 ) then
    propce(iel,ipckab) = ygfm*ckabgf + ygbm*ckabgb
    propce(iel,ipt4)   = ygfm*tgf**4 + ygbm*tgb**4
    propce(iel,ipt3)   = ygfm*tgf**3 + ygbm*tgb**3
  endif

enddo


!===============================================================================
! 3. CALCUL DE RHO ET DES FRACTIONS MASSIQUES DES ESPECES GLOBALES
!    SUR LES BORDS
!===============================================================================

! --> Masse volumique au bord

ibrom = 1

! ---- Masse volumique au bord pour toutes les facettes
!      Les facettes d'entree seront recalculees apres
!      a partir des CL (si IPASS > 2).

! ---- Au premier passage sans suite ou si on n'a pas relu la
!      masse volumique dans le fichier suite, on n'a pas recalcule la
!      masse volumique ci-dessus, pas la peine de la reprojeter aux
!      faces.

if (ipass.gt.1.or.(isuite.eq.1.and.initro.eq.1)) then

  do ifac = 1, nfabor
    iel = ifabor(ifac)
    propfb(ifac,ipbrom) = propce(iel,ipcrom)
  enddo

endif


! ---- Masse volumique au bord pour les facettes d'entree UNIQUEMENT
!      Le test sur IZONE sert pour les reprises de calcul
!      On suppose implicitement que les elements ci-dessus ont ete relus
!      dans le fichier suite (i.e. pas de suite en combustion d'un calcul
!      a froid) -> sera pris en compte eventuellement dans les versions
!      suivantes

if ( ipass.gt.1 .or. isuite.eq.1 ) then
  do ifac = 1, nfabor
    izone = izfppp(ifac)
    if(izone.gt.0) then
      if ( ientgb(izone).eq.1 .or. ientgf(izone).eq.1 ) then
        coefg(1) = fment(izone)
        coefg(2) = 1.d0-fment(izone)
        coefg(3) = zero
        if ( ientgb(izone).eq.1 ) then
          coefg(1) = max(zero,(fment(izone)-fs(1))/(1.d0-fs(1)))
          coefg(3) = (fment(izone)-coefg(1))/fs(1)
          coefg(2) = 1.d0 - coefg(1) - coefg(3)
        endif
        nbmol = 0.d0
        do igg = 1, ngazg
          nbmol = nbmol + coefg(igg)/wmolg(igg)
        enddo
       masmg = 1.d0/nbmol
       temsmm = tkent(izone)/masmg
       propfb(ifac,ipbrom) = p0/(rr*temsmm)
      endif
    endif
  enddo
endif


! --> Fractions massiques des especes globales au bord
!     Uniquement si transport de H

if ( iirayo.gt.0 ) then
  do igg = 1, ngazg
    ipbycg = ipprob(iym(igg))
    ipcycg = ipproc(iym(igg))
    do ifac = 1, nfabor
      iel = ifabor(ifac)
      propfb(ifac,ipbycg) = propce(iel,ipcycg)
    enddo
  enddo
endif

!===============================================================================
! FORMATS
!----


!----
! FIN
!----

return
end subroutine
