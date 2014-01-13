/*============================================================================
 * Functions dealing with ghost cells
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

#include "cs_defs.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "bft_mem.h"
#include "bft_error.h"
#include "bft_printf.h"

#include "cs_base.h"
#include "cs_order.h"

#include "cs_interface.h"
#include "fvm_periodicity.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_halo.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Static global variables
 *============================================================================*/

/* Number of defined halos */

static int _cs_glob_n_halos = 0;

#if defined(HAVE_MPI)

/* Send buffer for synchronization */

static size_t _cs_glob_halo_send_buffer_size = 0;
static void  *_cs_glob_halo_send_buffer = NULL;

/* MPI Request and status arrays */

static int           _cs_glob_halo_request_size = 0;
static MPI_Request  *_cs_glob_halo_request = NULL;
static MPI_Status   *_cs_glob_halo_status = NULL;

#endif

/* Buffer to save rotation halo values */

static size_t  _cs_glob_halo_rot_backup_size = 0;
static cs_real_t  *_cs_glob_halo_rot_backup = NULL;

/* Should we use barriers after posting receives ? */

static int _cs_glob_halo_use_barrier = false;

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Save rotation terms of a halo to an internal buffer.
 *
 * parameters:
 *   halo      <-- pointer to halo structure
 *   sync_mode <-- kind of halo treatment (standard or extended)
 *   stride    <-- number of (interlaced) values by entity
 *   var       <-- variable whose halo rotation terms are to be saved
 *                 (size: halo->n_local_elts + halo->n_elts[opt_type])
 *----------------------------------------------------------------------------*/

static void
_save_rotation_values(const cs_halo_t  *halo,
                      cs_halo_type_t    sync_mode,
                      int               stride,
                      const cs_real_t   var[])
{
  cs_lnum_t  i, j, rank_id, shift, t_id;
  cs_lnum_t  start_std, end_std, length, start_ext, end_ext;

  size_t  save_count = 0;

  cs_real_t  *save_buffer = _cs_glob_halo_rot_backup;

  const int  n_transforms = halo->n_transforms;
  const cs_lnum_t  n_elts  = halo->n_local_elts;
  const fvm_periodicity_t *periodicity = halo->periodicity;

  assert(halo != NULL);

  if (sync_mode == CS_HALO_N_TYPES)
    return;

  /* Loop on transforms */

  for (t_id = 0; t_id < n_transforms; t_id++) {

    shift = 4 * halo->n_c_domains * t_id;

    if (   fvm_periodicity_get_type(periodicity, t_id)
        >= FVM_PERIODICITY_ROTATION) {

      for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

        start_std = n_elts + halo->perio_lst[shift + 4*rank_id];
        length = halo->perio_lst[shift + 4*rank_id + 1];
        end_std = start_std + length;

        for (i = start_std; i < end_std; i++) {
          for (j = 0; j < stride; j++)
            save_buffer[save_count++] = var[i*stride + j];
        }

        if (sync_mode == CS_HALO_EXTENDED) {

          start_ext = n_elts + halo->perio_lst[shift + 4*rank_id + 2];
          length = halo->perio_lst[shift + 4*rank_id + 3];
          end_ext = start_ext + length;

          for (i = start_ext; i < end_ext; i++) {
            for (j = 0; j < stride; j++)
              save_buffer[save_count++] = var[i*stride + j];
          }

        }

      }

    } /* End if perio_type >= FVM_PERIODICITY_ROTATION) */

  }
}

/*----------------------------------------------------------------------------
 * Restore rotation terms of a halo from an internal buffer.
 *
 * parameters:
 *   halo      <-- pointer to halo structure
 *   sync_mode <-- kind of halo treatment (standard or extended)
 *   stride    <-- number of (interlaced) values by entity
 *   var       <-> variable whose halo rotation terms are to be restored
 *----------------------------------------------------------------------------*/

static void
_restore_rotation_values(const cs_halo_t  *halo,
                         cs_halo_type_t    sync_mode,
                         int               stride,
                         cs_real_t         var[])
{
  cs_lnum_t  i, j, rank_id, shift, t_id;
  cs_lnum_t  start_std, end_std, length, start_ext, end_ext;

  size_t restore_count = 0;

  const cs_real_t  *save_buffer = _cs_glob_halo_rot_backup;
  const int  n_transforms = halo->n_transforms;
  const cs_lnum_t  n_elts  = halo->n_local_elts;
  const fvm_periodicity_t *periodicity = halo->periodicity;

  if (sync_mode == CS_HALO_N_TYPES)
    return;

  assert(halo != NULL);

  /* Loop on transforms */

  for (t_id = 0; t_id < n_transforms; t_id++) {

    shift = 4 * halo->n_c_domains * t_id;

    if (   fvm_periodicity_get_type(periodicity, t_id)
        >= FVM_PERIODICITY_ROTATION) {

      for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

        start_std = n_elts + halo->perio_lst[shift + 4*rank_id];
        length = halo->perio_lst[shift + 4*rank_id + 1];
        end_std = start_std + length;

        for (i = start_std; i < end_std; i++) {
          for (j = 0; j < stride; j++)
            var[i*stride + j] = save_buffer[restore_count++];
        }

        if (sync_mode == CS_HALO_EXTENDED) {

          start_ext = n_elts + halo->perio_lst[shift + 4*rank_id + 2];
          length = halo->perio_lst[shift + 4*rank_id + 3];
          end_ext = start_ext + length;

          for (i = start_ext; i < end_ext; i++) {
            for (j = 0; j < stride; j++)
              var[i*stride + j] = save_buffer[restore_count++];
          }

        }

      }

    } /* End if perio_type >= FVM_PERIODICITY_ROTATION) */

  }
}

/*----------------------------------------------------------------------------
 * Set rotation terms of a halo to zero.
 *
 * parameters:
 *   halo      <-- pointer to halo structure
 *   sync_mode <-- kind of halo treatment (standard or extended)
 *   stride    <-- number of (interlaced) values by entity
 *   var       <-> variable whose halo rotation terms are to be zeroed
 *----------------------------------------------------------------------------*/

static void
_zero_rotation_values(const cs_halo_t  *halo,
                      cs_halo_type_t    sync_mode,
                      int               stride,
                      cs_real_t         var[])
{
  cs_lnum_t  i, j, rank_id, shift, t_id;
  cs_lnum_t  start_std, end_std, length, start_ext, end_ext;

  const int  n_transforms = halo->n_transforms;
  const cs_lnum_t  n_elts  = halo->n_local_elts;
  const fvm_periodicity_t *periodicity = halo->periodicity;

  if (sync_mode == CS_HALO_N_TYPES)
    return;

  assert(halo != NULL);

  /* Loop on transforms */

  for (t_id = 0; t_id < n_transforms; t_id++) {

    shift = 4 * halo->n_c_domains * t_id;

    if (   fvm_periodicity_get_type(periodicity, t_id)
        >= FVM_PERIODICITY_ROTATION) {

      for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

        start_std = n_elts + halo->perio_lst[shift + 4*rank_id];
        length = halo->perio_lst[shift + 4*rank_id + 1];
        end_std = start_std + length;

        for (i = start_std; i < end_std; i++) {
          for (j = 0; j < stride; j++)
            var[i*stride + j] = 0.0;
        }

        if (sync_mode == CS_HALO_EXTENDED) {

          start_ext = n_elts + halo->perio_lst[shift + 4*rank_id + 2];
          length = halo->perio_lst[shift + 4*rank_id + 3];
          end_ext = start_ext + length;

          for (i = start_ext; i < end_ext; i++) {
            for (j = 0; j < stride; j++)
              var[i*stride + j] = 0.0;
          }

        }

      }

    } /* End if perio_type >= FVM_PERIODICITY_ROTATION) */

  }
}

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Create a halo structure.
 *
 * parameters:
 *   ifs  <--  pointer to a cs_interface_set structure
 *
 * returns:
 *  pointer to created cs_halo_t structure
 *---------------------------------------------------------------------------*/

cs_halo_t *
cs_halo_create(cs_interface_set_t  *ifs)
{
  cs_lnum_t  i, tmp_id, perio_lst_size;

  cs_lnum_t  loc_id = -1;

  cs_halo_t  *halo = NULL;

  const cs_interface_t  *interface = NULL;

  BFT_MALLOC(halo, 1, cs_halo_t);

  halo->n_c_domains = cs_interface_set_size(ifs);
  halo->n_transforms = 0;

  halo->periodicity = cs_interface_set_periodicity(ifs);
  halo->n_rotations = 0;

  halo->n_local_elts = 0;

  for (i = 0; i < CS_HALO_N_TYPES; i++) {
    halo->n_send_elts[i] = 0;
    halo->n_elts [i] = 0;
  }

  BFT_MALLOC(halo->c_domain_rank, halo->n_c_domains, int);

  /* Check if cs_glob_rank_id belongs to interface set in order to
     order ranks with local rank at first place */

  for (i = 0; i < halo->n_c_domains; i++) {

    interface = cs_interface_set_get(ifs, i);
    halo->c_domain_rank[i] = cs_interface_rank(interface);

    if (cs_glob_rank_id == cs_interface_rank(interface))
      loc_id = i;

  } /* End of loop on ranks */

  if (loc_id > 0) {

    tmp_id = halo->c_domain_rank[loc_id];
    halo->c_domain_rank[loc_id] = halo->c_domain_rank[0];
    halo->c_domain_rank[0] = tmp_id;

  }

  /* Order ranks */

  if (   halo->n_c_domains > 2
      && cs_order_gnum_test(&(halo->c_domain_rank[1]),
                            NULL,
                            halo->n_c_domains-1) == 0) {

    cs_lnum_t  *order = NULL;
    cs_gnum_t  *buffer = NULL;

    BFT_MALLOC(order, halo->n_c_domains - 1, cs_lnum_t);
    BFT_MALLOC(buffer, halo->n_c_domains - 1, cs_gnum_t);

    for (i = 1; i < halo->n_c_domains; i++)
      buffer[i-1] = (cs_gnum_t)halo->c_domain_rank[i];

    cs_order_gnum_allocated(NULL,
                            buffer,
                            order,
                            halo->n_c_domains - 1);

    for (i = 0; i < halo->n_c_domains - 1; i++)
      halo->c_domain_rank[i+1] = (cs_lnum_t)buffer[order[i]];

    BFT_FREE(buffer);
    BFT_FREE(order);

  } /* End of ordering ranks */

  BFT_MALLOC(halo->send_index, 2*halo->n_c_domains + 1, cs_lnum_t);
  BFT_MALLOC(halo->index, 2*halo->n_c_domains + 1, cs_lnum_t);

  for (i = 0; i < 2*halo->n_c_domains + 1; i++) {
    halo->send_index[i] = 0;
    halo->index[i] = 0;
  }

  halo->send_perio_lst = NULL;
  halo->perio_lst = NULL;

  if (halo->periodicity != NULL) {

    halo->n_transforms = fvm_periodicity_get_n_transforms(halo->periodicity);

    for (i = 0; i < halo->n_transforms; i++) {
      if (   fvm_periodicity_get_type(halo->periodicity, i)
          >= FVM_PERIODICITY_ROTATION)
        halo->n_rotations += 1;
    }

    /* We need 2 values per transformation and there are n_transforms
       transformations. For each rank, we need a value for standard and
       extended halo. */

    perio_lst_size = 2*halo->n_transforms * 2*halo->n_c_domains;

    BFT_MALLOC(halo->send_perio_lst, perio_lst_size, cs_lnum_t);
    BFT_MALLOC(halo->perio_lst, perio_lst_size, cs_lnum_t);

    for (i = 0; i < perio_lst_size; i++) {
      halo->send_perio_lst[i] = 0;
      halo->perio_lst[i] = 0;
    }

  }

  halo->send_list = NULL;

  _cs_glob_n_halos += 1;

  return halo;
}

/*----------------------------------------------------------------------------
 * Create a halo structure, using a reference halo
 *
 * parameters:
 *   ref  <--  pointer to reference halo
 *
 * returns:
 *  pointer to created cs_halo_t structure
 *---------------------------------------------------------------------------*/

cs_halo_t *
cs_halo_create_from_ref(const cs_halo_t  *ref)
{
  cs_lnum_t  i;

  cs_halo_t  *halo = NULL;

  BFT_MALLOC(halo, 1, cs_halo_t);

  halo->n_c_domains = ref->n_c_domains;
  halo->n_transforms = ref->n_transforms;

  halo->periodicity = ref->periodicity;
  halo->n_rotations = ref->n_rotations;

  halo->n_local_elts = 0;

  BFT_MALLOC(halo->c_domain_rank, halo->n_c_domains, int);

  for (i = 0; i < halo->n_c_domains; i++)
    halo->c_domain_rank[i] = ref->c_domain_rank[i];

  BFT_MALLOC(halo->send_index, 2*halo->n_c_domains + 1, cs_lnum_t);
  BFT_MALLOC(halo->index, 2*halo->n_c_domains + 1, cs_lnum_t);

  for (i = 0; i < 2*halo->n_c_domains + 1; i++) {
    halo->send_index[i] = 0;
    halo->index[i] = 0;
  }

  halo->send_perio_lst = NULL;
  halo->perio_lst = NULL;

  if (halo->n_transforms > 0) {

    cs_lnum_t  perio_lst_size = 2*halo->n_transforms * 2*halo->n_c_domains;

    BFT_MALLOC(halo->send_perio_lst, perio_lst_size, cs_lnum_t);
    BFT_MALLOC(halo->perio_lst, perio_lst_size, cs_lnum_t);

    for (i = 0; i < perio_lst_size; i++) {
      halo->send_perio_lst[i] = 0;
      halo->perio_lst[i] = 0;
    }

  }

  halo->send_list = NULL;

  _cs_glob_n_halos += 1;

  return halo;
}

/*----------------------------------------------------------------------------
 * Destroy a halo structure
 *
 * parameters:
 *   halo  <--  pointer to cs_halo_t structure to destroy
 *
 * Returns:
 *  pointer to deleted halo structure (NULL)
 *---------------------------------------------------------------------------*/

cs_halo_t *
cs_halo_destroy(cs_halo_t  *halo)
{
  if (halo == NULL)
    return NULL;

  halo->n_c_domains = 0;
  BFT_FREE(halo->c_domain_rank);

  BFT_FREE(halo->send_perio_lst);
  BFT_FREE(halo->send_index);
  BFT_FREE(halo->perio_lst);
  BFT_FREE(halo->index);

  if (halo->send_list != NULL)
    BFT_FREE(halo->send_list);

  BFT_FREE(halo);

  _cs_glob_n_halos -= 1;

  /* Delete buffers if no halo remains */

  if (_cs_glob_n_halos == 0) {

#if defined(HAVE_MPI)

    if (cs_glob_n_ranks > 1) {

      _cs_glob_halo_send_buffer_size = 0;
      _cs_glob_halo_request_size = 0;

      BFT_FREE(_cs_glob_halo_send_buffer);

      BFT_FREE(_cs_glob_halo_request);
      BFT_FREE(_cs_glob_halo_status);
    }

#endif

  }

  return NULL;
}

/*----------------------------------------------------------------------------
 * Update global buffer sizes so as to be usable with a given halo.
 *
 * Calls to halo synchronizations with variable strides up to 3 are
 * expected. For strides greater than 3, the halo will be resized if
 * necessary directly by the synchronization function.
 *
 * This function should be called at the end of any halo creation,
 * so that buffer sizes are increased if necessary.
 *
 * parameters:
 *   halo <-- pointer to cs_halo_t structure.
 *---------------------------------------------------------------------------*/

void
cs_halo_update_buffers(const cs_halo_t *halo)
{
  if (halo == NULL)
    return;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {

    size_t send_buffer_size =   CS_MAX(halo->n_send_elts[CS_HALO_EXTENDED],
                                       halo->n_elts[CS_HALO_EXTENDED])
                              * CS_MAX(sizeof(cs_lnum_t), sizeof(cs_real_t)) * 3;

    int n_requests = halo->n_c_domains*2;

    if (send_buffer_size > _cs_glob_halo_send_buffer_size) {
      _cs_glob_halo_send_buffer_size =  send_buffer_size;
      BFT_REALLOC(_cs_glob_halo_send_buffer,
                  _cs_glob_halo_send_buffer_size,
                  char);
    }

    if (n_requests > _cs_glob_halo_request_size) {
      _cs_glob_halo_request_size = n_requests;
      BFT_REALLOC(_cs_glob_halo_request,
                  _cs_glob_halo_request_size,
                  MPI_Request);
      BFT_REALLOC(_cs_glob_halo_status,
                  _cs_glob_halo_request_size,
                  MPI_Status);

    }

  }

#endif

  /* Buffer to save and restore rotation halo values */

  if (halo->n_rotations > 0) {

    int rank_id, t_id, shift;
    size_t save_count = 0;

    const fvm_periodicity_t *periodicity = halo->periodicity;

    /* Loop on transforms */

    for (t_id = 0; t_id < halo->n_transforms; t_id++) {

      shift = 4 * halo->n_c_domains * t_id;

      if (   fvm_periodicity_get_type(periodicity, t_id)
          >= FVM_PERIODICITY_ROTATION) {
        for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {
          save_count += halo->perio_lst[shift + 4*rank_id + 1];
          save_count += halo->perio_lst[shift + 4*rank_id + 3];
        }
      }

    }

    save_count *= 3;

    if (save_count > _cs_glob_halo_rot_backup_size) {
      _cs_glob_halo_rot_backup_size = save_count;
      BFT_REALLOC(_cs_glob_halo_rot_backup,
                  _cs_glob_halo_rot_backup_size,
                  cs_real_t);
    }

  } /* End of test on presence of rotations */
}

/*----------------------------------------------------------------------------
 * Free global halo backup buffer.
 *---------------------------------------------------------------------------*/

void
cs_halo_free_buffer(void)
{
  if (_cs_glob_halo_rot_backup != NULL) {
    _cs_glob_halo_rot_backup_size = 0;
    BFT_FREE(_cs_glob_halo_rot_backup);
  }
}

/*----------------------------------------------------------------------------
 * Apply cell renumbering to a halo
 *
 * parameters:
 *   halo        <-- pointer to halo structure
 *   new_cell_id <-- array indicating old -> new cell id (0 to n-1)
 *---------------------------------------------------------------------------*/

void
cs_halo_renumber_cells(cs_halo_t        *halo,
                       const cs_lnum_t   new_cell_id[])
{
  if (halo != NULL) {

    cs_lnum_t i;
    const cs_lnum_t n_elts = halo->n_send_elts[CS_HALO_EXTENDED];

    for (i = 0; i < n_elts; i++)
      halo->send_list[i] = new_cell_id[halo->send_list[i]];
  }
}

/*----------------------------------------------------------------------------
 * Update array of any type of halo values in case of parallelism or
 * periodicity.
 *
 * Data is untyped; only its size is given, so this function may also
 * be used to synchronize interleaved multidimendsional data, using
 * size = element_size*dim (assuming a homogeneous environment, at least
 * as far as data encoding goes).
 *
 * This function aims at copying main values from local elements
 * (id between 1 and n_local_elements) to ghost elements on distant ranks
 * (id between n_local_elements + 1 to n_local_elements_with_halo).
 *
 * parameters:
 *   halo      <-- pointer to halo structure
 *   sync_mode <-- synchronization mode (standard or extended)
 *   size      <-- datatype size
 *   num       <-> pointer to local number value array
 *----------------------------------------------------------------------------*/

void
cs_halo_sync_untyped(const cs_halo_t  *halo,
                     cs_halo_type_t    sync_mode,
                     size_t            size,
                     void             *val)
{
  cs_lnum_t i, start, length;
  size_t j;

  cs_lnum_t end_shift = 0;
  int local_rank_id = (cs_glob_n_ranks == 1) ? 0 : -1;
  unsigned char *src;
  unsigned char *restrict _val = val;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {
    const size_t send_buffer_size =   CS_MAX(halo->n_send_elts[CS_HALO_EXTENDED],
                                             halo->n_elts[CS_HALO_EXTENDED])
                                    * size;

    if (send_buffer_size > _cs_glob_halo_send_buffer_size) {
      _cs_glob_halo_send_buffer_size =  send_buffer_size;
      BFT_REALLOC(_cs_glob_halo_send_buffer,
                  _cs_glob_halo_send_buffer_size,
                  char);
    }
  }

#endif /* defined(HAVE_MPI) */

  if (sync_mode == CS_HALO_STANDARD)
    end_shift = 1;

  else if (sync_mode == CS_HALO_EXTENDED)
    end_shift = 2;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {

    int rank_id;
    int request_count = 0;
    unsigned char *build_buffer = (unsigned char *)_cs_glob_halo_send_buffer;
    const int local_rank = cs_glob_rank_id;

    /* Receive data from distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      start = halo->index[2*rank_id];
      length = (  halo->index[2*rank_id + end_shift]
                - halo->index[2*rank_id]);

      if (halo->c_domain_rank[rank_id] != local_rank) {

        unsigned char *dest = _val + (halo->n_local_elts*size) + start*size;

        MPI_Irecv(dest,
                  length * size,
                  MPI_UNSIGNED_CHAR,
                  halo->c_domain_rank[rank_id],
                  halo->c_domain_rank[rank_id],
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }
      else
        local_rank_id = rank_id;

    }

    /* Assemble buffers for halo exchange;
       with threading, use dynamic scheduling ,as this is probably a small loop */

    #pragma omp parallel for private(start, length, src, i, j) schedule(dynamic)
    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length = (  halo->send_index[2*rank_id + end_shift]
                  - halo->send_index[2*rank_id]);

        src = build_buffer + start*size;

        for (i = 0; i < length; i++) {
          for (j = 0; j < size; j++)
            src[i*size + j] = _val[halo->send_list[start + i]*size + j];
        }

      }

    }

    /* We wait for posting all receives (often recommended) */

    if (_cs_glob_halo_use_barrier)
      MPI_Barrier(cs_glob_mpi_comm);

    /* Send data to distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      /* If this is not the local rank */

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length = (  halo->send_index[2*rank_id + end_shift]
                  - halo->send_index[2*rank_id]);

        MPI_Isend(build_buffer + start*size,
                  length*size,
                  MPI_UNSIGNED_CHAR,
                  halo->c_domain_rank[rank_id],
                  local_rank,
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }

    }

    /* Wait for all exchanges */

    MPI_Waitall(request_count, _cs_glob_halo_request, _cs_glob_halo_status);
  }

#endif /* defined(HAVE_MPI) */

  /* Copy local values in case of periodicity */

  if (halo->n_transforms > 0) {

    if (local_rank_id > -1) {

      unsigned char *recv
        = _val + (halo->n_local_elts + halo->index[2*local_rank_id]) * size;

      start = halo->send_index[2*local_rank_id];
      length = (  halo->send_index[2*local_rank_id + end_shift]
                - halo->send_index[2*local_rank_id]);

      for (i = 0; i < length; i++) {
        for (j = 0; j < size; j++)
          recv[i*size + j] = _val[halo->send_list[start + i]*size + j];
      }
    }

  }
}

/*----------------------------------------------------------------------------
 * Update array of integer halo values in case of parallelism or periodicity.
 *
 * This function aims at copying main values from local elements
 * (id between 1 and n_local_elements) to ghost elements on distant ranks
 * (id between n_local_elements + 1 to n_local_elements_with_halo).
 *
 * parameters:
 *   halo      <-- pointer to halo structure
 *   sync_mode <-- synchronization mode (standard or extended)
 *   num       <-> pointer to local number value array
 *----------------------------------------------------------------------------*/

void
cs_halo_sync_num(const cs_halo_t  *halo,
                 cs_halo_type_t    sync_mode,
                 cs_lnum_t         num[])
{
  cs_lnum_t i, start, length;

  cs_lnum_t end_shift = 0;
  int local_rank_id = (cs_glob_n_ranks == 1) ? 0 : -1;

  if (sync_mode == CS_HALO_STANDARD)
    end_shift = 1;

  else if (sync_mode == CS_HALO_EXTENDED)
    end_shift = 2;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {

    int rank_id;
    int request_count = 0;
    cs_lnum_t *build_buffer = (cs_lnum_t *)_cs_glob_halo_send_buffer;
    const int local_rank = cs_glob_rank_id;

    /* Receive data from distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      start = halo->index[2*rank_id];
      length = halo->index[2*rank_id + end_shift] - halo->index[2*rank_id];

      if (halo->c_domain_rank[rank_id] != local_rank) {

        MPI_Irecv(num + halo->n_local_elts + start,
                  length,
                  CS_MPI_INT,
                  halo->c_domain_rank[rank_id],
                  halo->c_domain_rank[rank_id],
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }
      else
        local_rank_id = rank_id;

    }

    /* Assemble buffers for halo exchange;
       with threading, use dynamic scheduling ,as this is probably a small loop */

    #pragma omp parallel for private(start, length, i) schedule(dynamic)
    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length =   halo->send_index[2*rank_id + end_shift]
                 - halo->send_index[2*rank_id];

        for (i = 0; i < length; i++)
          build_buffer[start + i] = num[halo->send_list[start + i]];

      }

    }

    /* We wait for posting all receives (often recommended) */

    if (_cs_glob_halo_use_barrier)
      MPI_Barrier(cs_glob_mpi_comm);

    /* Send data to distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length =   halo->send_index[2*rank_id + end_shift]
                 - halo->send_index[2*rank_id];

        MPI_Isend(build_buffer + start,
                  length,
                  CS_MPI_INT,
                  halo->c_domain_rank[rank_id],
                  local_rank,
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }

    }

    /* Wait for all exchanges */

    MPI_Waitall(request_count, _cs_glob_halo_request, _cs_glob_halo_status);
  }

#endif /* defined(HAVE_MPI) */

  /* Copy local values in case of periodicity */

  if (halo->n_transforms > 0) {

    if (local_rank_id > -1) {

      cs_lnum_t *recv_num
        = num + halo->n_local_elts + halo->index[2*local_rank_id];

      start = halo->send_index[2*local_rank_id];
      length =   halo->send_index[2*local_rank_id + end_shift]
               - halo->send_index[2*local_rank_id];

      for (i = 0; i < length; i++)
        recv_num[i] = num[halo->send_list[start + i]];

    }

  }
}

/*----------------------------------------------------------------------------
 * Update array of variable (floating-point) halo values in case of
 * parallelism or periodicity.
 *
 * This function aims at copying main values from local elements
 * (id between 1 and n_local_elements) to ghost elements on distant ranks
 * (id between n_local_elements + 1 to n_local_elements_with_halo).
 *
 * parameters:
 *   halo      <-- pointer to halo structure
 *   sync_mode <-- synchronization mode (standard or extended)
 *   var       <-> pointer to variable value array
 *----------------------------------------------------------------------------*/

void
cs_halo_sync_var(const cs_halo_t  *halo,
                 cs_halo_type_t    sync_mode,
                 cs_real_t         var[])
{
  cs_lnum_t i, start, length;

  int local_rank_id = (cs_glob_n_ranks == 1) ? 0 : -1;
  const cs_lnum_t end_shift = (sync_mode == CS_HALO_STANDARD) ? 1 : 2;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {

    int rank_id;
    int request_count = 0;
    cs_real_t *build_buffer = (cs_real_t *)_cs_glob_halo_send_buffer;
    const int local_rank = cs_glob_rank_id;

    /* Receive data from distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      start = halo->index[2*rank_id];
      length = halo->index[2*rank_id + end_shift] - halo->index[2*rank_id];

      if (halo->c_domain_rank[rank_id] != local_rank) {

        MPI_Irecv(var + halo->n_local_elts + start,
                  length,
                  CS_MPI_REAL,
                  halo->c_domain_rank[rank_id],
                  halo->c_domain_rank[rank_id],
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }
      else
        local_rank_id = rank_id;

    }

    /* Assemble buffers for halo exchange;
       with threading, use dynamic scheduling ,as this is probably a small loop */

    #pragma omp parallel for private(start, length, i) schedule(dynamic)
    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length =   halo->send_index[2*rank_id + end_shift]
                 - halo->send_index[2*rank_id];

        for (i = 0; i < length; i++)
          build_buffer[start + i] = var[halo->send_list[start + i]];

      }

    }

    /* We wait for posting all receives (often recommended) */

    if (_cs_glob_halo_use_barrier)
      MPI_Barrier(cs_glob_mpi_comm);

    /* Send data to distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length =   halo->send_index[2*rank_id + end_shift]
                 - halo->send_index[2*rank_id];

        MPI_Isend(build_buffer + start,
                  length,
                  CS_MPI_REAL,
                  halo->c_domain_rank[rank_id],
                  local_rank,
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }

    }

    /* Wait for all exchanges */

    MPI_Waitall(request_count, _cs_glob_halo_request, _cs_glob_halo_status);
  }

#endif /* defined(HAVE_MPI) */

  /* Copy local values in case of periodicity */

  if (halo->n_transforms > 0) {

    if (local_rank_id > -1) {

      cs_real_t *recv_var
        = var + halo->n_local_elts + halo->index[2*local_rank_id];

      start = halo->send_index[2*local_rank_id];
      length =   halo->send_index[2*local_rank_id + end_shift]
               - halo->send_index[2*local_rank_id];

      for (i = 0; i < length; i++)
        recv_var[i] = var[halo->send_list[start + i]];

    }

  }
}

/*----------------------------------------------------------------------------
 * Update array of strided variable (floating-point) values in case
 * of parallelism or periodicity.
 *
 * This function aims at copying main values from local elements
 * (id between 1 and n_local_elements) to ghost elements on distant ranks
 * (id between n_local_elements + 1 to n_local_elements_with_halo).
 *
 * parameters:
 *   halo      <-- pointer to halo structure
 *   sync_mode <-- synchronization mode (standard or extended)
 *   var       <-> pointer to variable value array
 *   stride    <-- number of (interlaced) values by entity
 *----------------------------------------------------------------------------*/

void
cs_halo_sync_var_strided(const cs_halo_t  *halo,
                         cs_halo_type_t    sync_mode,
                         cs_real_t         var[],
                         int               stride)
{
  cs_lnum_t i, j, start, length;

  cs_lnum_t end_shift = 0;
  int local_rank_id = (cs_glob_n_ranks == 1) ? 0 : -1;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {
    const size_t send_buffer_size =   CS_MAX(halo->n_send_elts[CS_HALO_EXTENDED],
                                             halo->n_elts[CS_HALO_EXTENDED])
                                    * sizeof(cs_real_t) * stride;

    if (send_buffer_size > _cs_glob_halo_send_buffer_size) {
      _cs_glob_halo_send_buffer_size =  send_buffer_size;
      BFT_REALLOC(_cs_glob_halo_send_buffer,
                  _cs_glob_halo_send_buffer_size,
                  char);
    }
  }

#endif /* defined(HAVE_MPI) */

  if (sync_mode == CS_HALO_STANDARD)
    end_shift = 1;

  else if (sync_mode == CS_HALO_EXTENDED)
    end_shift = 2;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {

    int rank_id;
    int request_count = 0;
    cs_real_t *build_buffer = (cs_real_t *)_cs_glob_halo_send_buffer;
    cs_real_t *buffer = NULL;
    const int local_rank = cs_glob_rank_id;

    /* Receive data from distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      length = (  halo->index[2*rank_id + end_shift]
                - halo->index[2*rank_id]) * stride;

      if (halo->c_domain_rank[rank_id] != local_rank) {

        buffer = var + (halo->n_local_elts + halo->index[2*rank_id])*stride;

        MPI_Irecv(buffer,
                  length,
                  CS_MPI_REAL,
                  halo->c_domain_rank[rank_id],
                  halo->c_domain_rank[rank_id],
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }
      else
        local_rank_id = rank_id;

    }

    /* Assemble buffers for halo exchange;
       with threading, use dynamic scheduling ,as this is probably a small loop */

    #pragma omp parallel for private(start, length, i, j) schedule(dynamic)
    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length = (  halo->send_index[2*rank_id + end_shift]
                  - halo->send_index[2*rank_id]);

        if (stride == 3) { /* Unroll loop for this case */
          for (i = 0; i < length; i++) {
            build_buffer[(start + i)*3]
              = var[(halo->send_list[start + i])*3];
            build_buffer[(start + i)*3 + 1]
              = var[(halo->send_list[start + i])*3 + 1];
            build_buffer[(start + i)*3 + 2]
              = var[(halo->send_list[start + i])*3 + 2];
          }
        }
        else {
          for (i = 0; i < length; i++) {
            for (j = 0; j < stride; j++)
              build_buffer[(start + i)*stride + j]
                = var[(halo->send_list[start + i])*stride + j];
          }
        }

      }

    }

    /* We wait for posting all receives (often recommended) */

    if (_cs_glob_halo_use_barrier)
      MPI_Barrier(cs_glob_mpi_comm);

    /* Send data to distant ranks */

    for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

      /* If this is not the local rank */

      if (halo->c_domain_rank[rank_id] != local_rank) {

        start = halo->send_index[2*rank_id];
        length = (  halo->send_index[2*rank_id + end_shift]
                  - halo->send_index[2*rank_id]);

        MPI_Isend(build_buffer + start*stride,
                  length*stride,
                  CS_MPI_REAL,
                  halo->c_domain_rank[rank_id],
                  local_rank,
                  cs_glob_mpi_comm,
                  &(_cs_glob_halo_request[request_count++]));

      }

    }

    /* Wait for all exchanges */

    MPI_Waitall(request_count, _cs_glob_halo_request, _cs_glob_halo_status);
  }

#endif /* defined(HAVE_MPI) */

  /* Copy local values in case of periodicity */

  if (halo->n_transforms > 0) {

    if (local_rank_id > -1) {

      cs_real_t *recv_var
        = var + (halo->n_local_elts + halo->index[2*local_rank_id])*stride;

      start = halo->send_index[2*local_rank_id];
      length =   halo->send_index[2*local_rank_id + end_shift]
               - halo->send_index[2*local_rank_id];

      if (stride == 3) { /* Unroll loop for this case */
        for (i = 0; i < length; i++) {
          recv_var[i*3]     = var[(halo->send_list[start + i])*3];
          recv_var[i*3 + 1] = var[(halo->send_list[start + i])*3 + 1];
          recv_var[i*3 + 2] = var[(halo->send_list[start + i])*3 + 2];
        }
      }
      else {
        for (i = 0; i < length; i++) {
          for (j = 0; j < stride; j++)
            recv_var[i*stride + j]
              = var[(halo->send_list[start + i])*stride + j];
        }
      }

    }

  }
}

/*----------------------------------------------------------------------------
 * Update array of vector variable component (floating-point) halo values
 * in case of parallelism or periodicity.
 *
 * This function aims at copying main values from local elements
 * (id between 1 and n_local_elements) to ghost elements on distant ranks
 * (id between n_local_elements + 1 to n_local_elements_with_halo).
 *
 * If rotation_op is equal to CS_HALO_ROTATION_IGNORE, halo values
 * corresponding to periodicity with rotation are left unchanged from their
 * previous values.
 *
 * If rotation_op is set to CS_HALO_ROTATION_ZERO, halo values
 * corresponding to periodicity with rotation are set to 0.
 *
 * If rotation_op is equal to CS_HALO_ROTATION_COPY, halo values
 * corresponding to periodicity with rotation are exchanged normally, so
 * the behavior is the same as that of cs_halo_sync_var().
 *
 * parameters:
 *   halo        <-- pointer to halo structure
 *   sync_mode   <-- synchronization mode (standard or extended)
 *   rotation_op <-- rotation operation
 *   var         <-> pointer to variable value array
 *----------------------------------------------------------------------------*/

void
cs_halo_sync_component(const cs_halo_t    *halo,
                       cs_halo_type_t      sync_mode,
                       cs_halo_rotation_t  rotation_op,
                       cs_real_t           var[])
{
  if (   halo->n_rotations > 0
      && rotation_op == CS_HALO_ROTATION_IGNORE)
    _save_rotation_values(halo, sync_mode, 1, var);

  cs_halo_sync_var(halo, sync_mode, var);

  if (halo->n_rotations > 0) {
    if (rotation_op == CS_HALO_ROTATION_IGNORE)
      _restore_rotation_values(halo, sync_mode, 1, var);
    else if (rotation_op == CS_HALO_ROTATION_ZERO)
      _zero_rotation_values(halo, sync_mode, 1, var);
  }
}

/*----------------------------------------------------------------------------
 * Update array of strided vector variable components (floating-point)
 * halo values in case of parallelism or periodicity.
 *
 * This function aims at copying main values from local elements
 * (id between 1 and n_local_elements) to ghost elements on distant ranks
 * (id between n_local_elements + 1 to n_local_elements_with_halo).
 *
 * If rotation_op is equal to CS_HALO_ROTATION_IGNORE, halo values
 * corresponding to periodicity with rotation are left unchanged from their
 * previous values.
 *
 * If rotation_op is equal to CS_HALO_ROTATION_ZERO, halo values
 * corresponding to periodicity with rotation are set to 0.
 *
 * If rotation_op is equal to CS_HALO_ROTATION_COPY, halo values
 * corresponding to periodicity with rotation are exchanged normally, so
 * the behavior is the same as that of cs_halo_sync_var_strided().
 *
 * parameters:
 *   halo        <-- pointer to halo structure
 *   sync_mode   <-- synchronization mode (standard or extended)
 *   rotation_op <-- rotation operation
 *   var         <-> pointer to variable value array
 *   stride      <-- number of (interlaced) values by entity
 *----------------------------------------------------------------------------*/

void
cs_halo_sync_components_strided(const cs_halo_t    *halo,
                                cs_halo_type_t      sync_mode,
                                cs_halo_rotation_t  rotation_op,
                                cs_real_t           var[],
                                int                 stride)
{
  if (   halo->n_rotations > 0
      && rotation_op == CS_HALO_ROTATION_IGNORE)
    _save_rotation_values(halo, sync_mode, stride, var);

  cs_halo_sync_var_strided(halo, sync_mode, var, stride);

  if (halo->n_rotations > 0) {
    if (rotation_op == CS_HALO_ROTATION_IGNORE)
      _restore_rotation_values(halo, sync_mode, stride, var);
    else if (rotation_op == CS_HALO_ROTATION_ZERO)
      _zero_rotation_values(halo, sync_mode, stride, var);
  }

}

/*----------------------------------------------------------------------------
 * Return MPI_Barrier usage flag.
 *
 * returns:
 *   true if MPI barriers are used after posting receives and before posting
 *   sends, false otherwise
 *---------------------------------------------------------------------------*/

bool
cs_halo_get_use_barrier(void)
{
  return _cs_glob_halo_use_barrier;
}

/*----------------------------------------------------------------------------
 * Set MPI_Barrier usage flag.
 *
 * parameters:
 *   use_barrier <-- true if MPI barriers should be used after posting
 *                   receives and before posting sends, false otherwise.
 *---------------------------------------------------------------------------*/

void
cs_halo_set_use_barrier(bool use_barrier)
{
  _cs_glob_halo_use_barrier = use_barrier;
}

/*----------------------------------------------------------------------------
 * Dump a cs_halo_t structure.
 *
 * parameters:
 *   halo           <-- pointer to cs_halo_t struture
 *   print_level    <--  0 only dimensions and indexes are printed, else (1)
 *                       everything is printed
 *---------------------------------------------------------------------------*/

void
cs_halo_dump(const cs_halo_t  *halo,
             int               print_level)
{
  cs_lnum_t  i, j, halo_id;

  if (halo == NULL) {
    bft_printf("\n\n  halo: nil\n");
    return;
  }

  bft_printf("\n  halo:         %p\n"
             "  n_transforms:   %d\n"
             "  n_c_domains:    %d\n"
             "  periodicity:    %p\n"
             "  n_rotations:    %d\n"
             "  n_local_elts:   %d\n",
             (const void *)halo,
             halo->n_transforms, halo->n_c_domains,
             (const void *)halo->periodicity,
             halo->n_rotations, halo->n_local_elts);

  bft_printf("\nRanks on halo frontier:\n");
  for (i = 0; i < halo->n_c_domains; i++)
    bft_printf("%5d", halo->c_domain_rank[i]);

  for (halo_id = 0; halo_id < 2; halo_id++) {

    cs_lnum_t  n_elts[2];
    cs_lnum_t  *index = NULL, *list = NULL, *perio_lst = NULL;

    bft_printf("\n    ---------\n");

    if (halo_id == 0) {

      bft_printf("    send_list:\n");
      n_elts[0] = halo->n_send_elts[0];
      n_elts[1] = halo->n_send_elts[1];
      index = halo->send_index;
      list = halo->send_list;
      perio_lst = halo->send_perio_lst;

    }
    else if (halo_id == 1) {

      bft_printf("    halo:\n");
      n_elts[0] = halo->n_elts[0];
      n_elts[1] = halo->n_elts[1];
      index = halo->index;
      list = NULL;
      perio_lst = halo->perio_lst;

    }

    bft_printf("    ---------\n\n");
    bft_printf("  n_ghost_cells:        %d\n"
               "  n_std_ghost_cells:    %d\n", n_elts[1], n_elts[0]);

    if (index == NULL)
      return;

    if (halo->n_transforms > 0) {

      const cs_lnum_t  stride = 4*halo->n_c_domains;

      for (i = 0; i < halo->n_transforms; i++) {

        bft_printf("\nTransformation number: %d\n", i+1);

        for (j = 0; j < halo->n_c_domains; j++) {

          bft_printf("    rank %3d <STD> %5d %5d <EXT> %5d %5d\n",
                     halo->c_domain_rank[j],
                     perio_lst[i*stride + 4*j],
                     perio_lst[i*stride + 4*j+1],
                     perio_lst[i*stride + 4*j+2],
                     perio_lst[i*stride + 4*j+3]);
        }

      } /* End of loop on perio */

    } /* End if n_perio > 0 */

    for (i = 0; i < halo->n_c_domains; i++) {

      bft_printf("\n  rank      %d:\n", halo->c_domain_rank[i]);

      if (index[2*i+1] - index[2*i] > 0) {

        bft_printf("\n  Standard halo\n");
        bft_printf("  idx start %d:          idx end   %d:\n",
                   index[2*i], index[2*i+1]);

        if (print_level == 1 && list != NULL) {
          bft_printf("\n            id      cell number\n");
          for (j = index[2*i]; j < index[2*i+1]; j++)
            bft_printf("    %10d %10d\n", j, list[j]+1);
        }

      } /* there are elements on standard neighborhood */

      if (index[2*i+2] - index[2*i+1] > 0) {

        bft_printf("\n  Extended halo\n");
        bft_printf("  idx start %d:          idx end   %d:\n",
                   index[2*i+1], index[2*i+2]);

        if (print_level == 1 && list != NULL) {
          bft_printf("\n            id      cell number\n");
          for (j = index[2*i+1]; j < index[2*i+2]; j++)
            bft_printf("    %10d %10d %10d\n",
                       j, list[j]+1, halo->n_local_elts+j+1);
        }

      } /* If there are elements on extended neighborhood */

    } /* End of loop on involved ranks */

  } /* End of loop on halos (send_halo/halo) */

  bft_printf("\n\n");
  bft_printf_flush();
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
