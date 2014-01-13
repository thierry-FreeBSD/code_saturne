#ifndef __CS_PHYSICAL_PROPERTIES_H__
#define __CS_PHYSICAL_PROPERTIES_H__

/*============================================================================
 * Functions dealing with parallelism
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

#include "cs_defs.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

typedef enum {

  CS_PHYS_PROP_PLANE_PH,
  CS_PHYS_PROP_PLANE_PT,
  CS_PHYS_PROP_PLANE_PS,
  CS_PHYS_PROP_PLANE_PU,
  CS_PHYS_PROP_PLANE_PV,
  CS_PHYS_PROP_PLANE_TS,
  CS_PHYS_PROP_PLANE_TX,

} cs_phys_prop_thermo_plane_type_t;

typedef enum {

  CS_PHYS_PROP_PRESSURE,
  CS_PHYS_PROP_TEMPERATURE,
  CS_PHYS_PROP_ENTHALPY,
  CS_PHYS_PROP_ENTROPY,
  CS_PHYS_PROP_ISOBARIC_HEAT_CAPACITY,
  CS_PHYS_PROP_ISOCHORIC_HEAT_CAPACITY,
  CS_PHYS_PROP_SPECIFIC_VOLUME,
  CS_PHYS_PROP_DENSITY,
  CS_PHYS_PROP_INTERNAL_ENERGY,
  CS_PHYS_PROP_QUALITY,
  CS_PHYS_PROP_THERMAL_CONDUCTIVITY,
  CS_PHYS_PROP_DYNAMIC_VISCOSITY,
  CS_PHYS_PROP_SPEED_OF_SOUND

} cs_phys_prop_type_t;

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Compute properties with Freesteam in a defined thermal plane.
 *
 * parameters:
 *   thermo_plane <-- thermodynamic plane
 *   property     <-- property queried
 *   n_vals       <-- number of values
 *   var1         <-- values on first plane axis
 *   var2         <-- values on second plane axis
 *   val          --> resulting property values
 *
 * returns:
 *   floating point value associated with the key id for this field
 *----------------------------------------------------------------------------*/

void
cs_phys_prop_freesteam(cs_phys_prop_thermo_plane_type_t   thermo_plane,
                       cs_phys_prop_type_t                property,
                       const cs_lnum_t                    n_vals,
                       const cs_real_t                    var1[],
                       const cs_real_t                    var2[],
                       cs_real_t                          val[]);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_PHYSICAL_PROPERTIES_H__ */
