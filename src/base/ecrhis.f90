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

!> \file ecrhis.f90
!> \brief Write plot data
!>
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
! Arguments
!------------------------------------------------------------------------------
!   mode          name          role
!------------------------------------------------------------------------------
!> \param[in]     modhis        0 or 1: initialize/output; 2: finalize
!______________________________________________________________________________

subroutine ecrhis &
!================

 ( modhis )

!===============================================================================
! Module files
!===============================================================================

use paramx
use numvar
use entsor
use cstnum
use optcal
use parall
use mesh
use field

!===============================================================================

implicit none

! Arguments

integer          modhis

! Local variables

character        nompre*300, nomhis*300
logical          interleaved
integer          tplnum, ii, lpre, lnom, lng
integer          icap, ncap, ipp, ippf
integer          iel, keymom
integer          mom_id, c_id, f_id, f_dim, f_type, n_fields
integer          nbcap(nvppmx)
double precision varcap(ncaptm)

integer, dimension(:), allocatable :: lsttmp
double precision, dimension(:), allocatable, target :: momtmp
double precision, dimension(:,:), allocatable :: xyztmp

double precision, dimension(:,:), pointer :: val_v
double precision, dimension(:), pointer :: val_s
double precision, dimension(:), pointer :: num_s, div_s

character*3, dimension(3), save :: nomext3 = (/'[X]', '[Y]', '[Z]'/)
character*4, dimension(3), save :: nomext63 = (/'[11]', '[22]', '[33]'/)
integer, save :: keypp = -1

! Time plot number shift (in case multiple routines define plots)

integer  nptpl
data     nptpl /0/
save     nptpl

! Number of passes in this routine

integer  ipass
data     ipass /0/
save     ipass

!===============================================================================
! 0. Local initializations
!===============================================================================

ipass = ipass + 1

!--> If no probe data has been output or there are no probes, return
if ((ipass.eq.1 .and. modhis.eq.2) .or. ncapt.eq.0) return

if (ipass.eq.1) then
  call tplnbr(nptpl)
  !==========
endif

if (keypp.lt.0) then
  call field_get_key_id("post_id", keypp)
endif

!===============================================================================
! 1. Search for neighboring nodes -> nodcap
!===============================================================================

if (ipass.eq.1) then
  do ii = 1, ncapt
    call findpt                                                        &
    !==========
    (ncelet, ncel, xyzcen,                                             &
     xyzcap(1,ii), xyzcap(2,ii), xyzcap(3,ii), nodcap(ii), ndrcap(ii))
  enddo
endif

!===============================================================================
! 2. Initialize output
!===============================================================================

! Create directory if required
if (ipass.eq.1 .and. irangp.le.0) then
  call csmkdr(emphis, len(emphis))
  !==========
endif

if (ipass.eq.1) then

  ! plot prefix
  nompre = trim(adjustl(emphis)) // trim(adjustl(prehis))
  lpre = len_trim(nompre)

  ! Number of probes per variable
  do ipp = 2, nvppmx
    nbcap(ipp) = ihisvr(ipp,1)
    if (nbcap(ipp).lt.0) nbcap(ipp) = ncapt
  enddo

  allocate(lsttmp(ncapt))
  allocate(xyztmp(3, ncapt))

  ! Initialize one output per variable

  call field_get_n_fields(n_fields)

  do f_id = 0, n_fields - 1

    call field_get_key_int(f_id, keypp, ippf)
    if (ippf.le.1) cycle

    call field_get_dim (f_id, f_dim, interleaved)

    do c_id = 1, min(f_dim, 3)

      ipp = ippf + c_id - 1

      if (ihisvr(ipp,1) .gt. 0) then
        do ii=1, ihisvr(ipp,1)
          lsttmp(ii) = ihisvr(ipp,ii+1)
          if (irangp.lt.0 .or. irangp.eq.ndrcap(ihisvr(ipp, ii+1))) then
            xyztmp(1, ii) = xyzcen(1, nodcap(ihisvr(ipp, ii+1)))
            xyztmp(2, ii) = xyzcen(2, nodcap(ihisvr(ipp, ii+1)))
            xyztmp(3, ii) = xyzcen(3, nodcap(ihisvr(ipp, ii+1)))
          endif
          if (irangp.ge.0) then
            lng = 3
            call parbcr(ndrcap(ihisvr(ipp,ii+1)), lng , xyztmp(1, ii))
            !==========
          endif
        enddo
      else
        do ii = 1, nbcap(ipp)
          lsttmp(ii) = ii
          if (irangp.lt.0 .or. irangp.eq.ndrcap(ii)) then
            xyztmp(1, ii) = xyzcen(1, nodcap(ii))
            xyztmp(2, ii) = xyzcen(2, nodcap(ii))
            xyztmp(3, ii) = xyzcen(3, nodcap(ii))
          endif
          if (irangp.ge.0) then
            lng = 3
            call parbcr(ndrcap(ii), lng , xyztmp(1, ii))
            !==========
          endif
        enddo
      endif

      if (nbcap(ipp) .gt. 0) then

        if (irangp.le.0) then

          call field_get_label(f_id, nomhis)
          nomhis = adjustl(nomhis)
          if (f_dim .eq. 3) then
            nomhis = trim(nomhis) // nomext3(c_id)
          else if (f_dim .eq. 6) then
            nomhis = trim(nomhis) // nomext63(c_id)
          endif
          lnom = len_trim(nomhis)

          tplnum = nptpl + ipp
          call tppini(tplnum, nomhis, nompre, tplfmt, idtvar, nthsav, tplflw, &
          !==========
                      nbcap(ipp), lsttmp(1), xyzcap(1,1), lnom, lpre)

        endif ! (irangp.le.0)

      endif

    enddo

  enddo

  deallocate(lsttmp)
  deallocate(xyztmp)

endif

!===============================================================================
! 3. Output results
!===============================================================================

if (modhis.eq.0 .or. modhis.eq.1) then

  call field_get_n_fields(n_fields)
  call field_get_key_id('moment_dt', keymom)

  ! Loop on fields

  do f_id = 0, n_fields - 1

    call field_get_key_int(f_id, keypp, ippf)
    if (ippf.le.1) cycle

    call field_get_dim (f_id, f_dim, interleaved)
    call field_get_type(f_id, f_type)

    mom_id = -1
    if (iand(f_type, FIELD_PROPERTY) .eq. 1) then
      call field_get_key_int(f_id, keymom, mom_id)
    endif

    do c_id = 1, min(f_dim, 3)

      ipp = ippf + c_id - 1

      if (ihisvr(ipp,1).ne.0) then

        ! Case of 1D fields, including moments

        if (f_dim .eq. 1) then

          if (mom_id.eq.-1) then
            call field_get_val_s(f_id, val_s)
          else
            allocate(momtmp(ncel))
            val_s => momtmp
            call field_get_val_s(f_id, num_s)
            if (mom_id.ge.0) then
              call field_get_val_s(mom_id, div_s)
              do iel = 1, ncel
                momtmp(iel) = num_s(iel)/max(div_s(iel),epzero)
              enddo
            else
              do iel = 1, ncel
                momtmp(iel) = num_s(iel)/max(dtcmom(-mom_id -1),epzero)
              enddo
            endif
          endif

          if (ihisvr(ipp,1).lt.0) then
            do icap = 1, ncapt
              if (irangp.lt.0) then
                varcap(icap) = val_s(nodcap(icap))
              else
                call parhis(nodcap(icap), ndrcap(icap), val_s, varcap(icap))
                !==========
              endif
            enddo
            ncap = ncapt
          else
            do icap = 1, ihisvr(ipp,1)
              if (irangp.lt.0) then
                varcap(icap) = val_s(nodcap(ihisvr(ipp,icap+1)))
              else
                call parhis(nodcap(ihisvr(ipp,icap+1)), &
                !==========
                           ndrcap(ihisvr(ipp,icap+1)), &
                           val_s, varcap(icap))
              endif
            enddo
            ncap = ihisvr(ipp,1)
          endif

          if (mom_id.ne.-1) then
            deallocate(momtmp)
          endif

          if (irangp.le.0 .and. ncap.gt.0) then
            tplnum = nptpl + ipp
            call tplwri(tplnum, tplfmt, ncap, ntcabs, ttcabs, varcap)
            !==========
          endif

        else ! For vector field

          call field_get_val_v(f_id, val_v)

          if (ihisvr(ipp,1).lt.0) then
            do icap = 1, ncapt
              if (irangp.lt.0 .or. ndrcap(icap).eq.irangp) then
                if (interleaved) then
                  varcap(icap) = val_v(c_id, nodcap(icap))
                else
                  varcap(icap) = val_v(nodcap(icap), c_id)
                endif
              endif
              if (irangp.ge.0) then
                lng = 1
                call parbcr(ndrcap(icap), lng, varcap(icap))
                !==========
              endif
            enddo
            ncap = ncapt
          else if (ihisvr(ipp,1).gt.0) then
            do icap = 1, ihisvr(ipp,1)
              if (irangp.lt.0 .or. ndrcap(icap).eq.irangp) then
                if (interleaved) then
                  varcap(icap) = val_v(c_id, nodcap(ihisvr(ipp,icap+1)))
                else
                  varcap(icap) = val_v(nodcap(ihisvr(ipp,icap+1)), c_id)
                endif
              endif
              if (irangp.ge.0) then
                lng = 1
                call parbcr(ndrcap(icap), lng, varcap(icap))
                !==========
              endif
            enddo
            ncap = ihisvr(ipp,1)
          else
            ncap = 0
          endif

          if (irangp.le.0 .and. ncap.gt.0) then
            tplnum = nptpl + ipp
            call tplwri(tplnum, tplfmt, ncap, ntcabs, ttcabs, varcap)
            !==========
          endif

        endif ! Scalar or  vector field

      endif ! Output for this component

    enddo ! loop on components

  enddo ! loop on fields

 endif

!===============================================================================
! 4. Close output
!===============================================================================

if (modhis.eq.2) then

  do ipp = 2, nvppmx
    if (ihisvr(ipp,1).lt.0) then
      ncap = ncapt
    else
      ncap = ihisvr(ipp,1)
    endif
    if (irangp.le.0 .and. ncap.gt.0) then
      tplnum = nptpl + ipp
      call tplend(tplnum, tplfmt)
      !==========
    endif
  enddo

endif

!===============================================================================
! 5. End
!===============================================================================

return
end subroutine
