// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2016 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Nic Olsen
// =============================================================================

#ifndef CHRONO_DISTRIBUTED_PHYSICS_CHDOMAINDISTR_H_
#define CHRONO_DISTRIBUTED_PHYSICS_CHDOMAINDISTR_H_

#include <memory>

#include "chrono/core/ChVector.h"
#include "chrono/physics/ChBody.h"

#include "chrono_distributed/physics/ChSystemDistr.h"
#include "chrono_distributed/ChApiDistributed.h"
#include "chrono_distributed/other_types.h"

namespace chrono {

class ChSystemDistr;

/// This class maps sub-domains of the global simulation domain to each MPI rank.
/// The global domain is split along the longest axis.
/// Within each sub-domain, there are layers of ownership:
///
/// ---------------------------------------------------
/// 0 < RANK < NUM_RANKS - 1:
///
/// Unowned_up (high + ghostlayer <= pos)
/// Ghost_up (high <= pos < high + ghostlayer)
/// Shared_up (high - ghostlayer <= pos < high)
/// Owned (low + ghostlayer <= pos < high - ghostlayer)
/// Shared_down (low <= pos < low + ghostlayer)
/// Ghost_down (low - ghost_layer <= pos < low)
/// Unowned_down (pos < low - ghostlayer)
///
/// ---------------------------------------------------
/// RANK = 0:
///
/// Unowned_up (high + ghostlayer <= pos)
/// Ghost_up (high <= pos < high + ghostlayer)
/// Shared_up (high - ghostlayer <= pos < high)
/// Owned (low <= pos < high - ghostlayer)
/// Unowned_down (pos < low)
///
///----------------------------------------------------
/// RANK = NUM_RANKS - 1:
///
/// Unowned_up (high <= pos)
/// Owned (low + ghostlayer <= pos < high)
/// Shared_down (low <= pos < low + ghostlayer)
/// Ghost_down (low - ghost_layer <= pos < low)
/// Unowned_down (pos < low - ghostlayer)
///-----------------------------------------------------
///
/// At AddBody:
/// ** Unowned_up/Unowned_down:
/// 		Bodies in this region do not interact with this rank
/// ** Ghost_up/Ghost_down:
/// 		Bodies in these regions are added to this rank, and expect to be updated
/// 		by a neighbor rank every timestep
/// ** Shared_up/Shared_down:
/// 		Bodies in these regions are added to this rank, and send updates to a neighbor
/// 		rank every timestep
/// ** Owned:
/// 		Bodies in this region are added to this rank only and have no interaction with other ranks.
///
///
/// Mid-Simulation:
/// ** Unowned_up/Unowned_down:
/// 		Bodies in these regions do not interact with this rank
/// ** Ghost_up/Ghost_down:
/// 		--If a body in one of these regions was most recently in this rank, it will be simulated on this
/// 		rank and sent to update a ghost body on a neighbor rank every timestep.
/// 		--If a body in one of these regions was most recently in another rank, it will be simulated on this
/// 		rank and will be updated by a neighbor rank every timestep.
/// ** Shared_up/Shared_down:
/// 		--If a body in one of these regions was most recently in this rank, it will be simulated on this
/// 		rank and sent to update a ghost body on a neighbor rank every timestep.
/// 		--If a body in one of these regions was most recently in another rank, it will be simulated on this
/// 		rank and will be updated by a neighbor rank every timestep.
/// ** Owned:
/// 		Bodies in this region are simulated only on this rank.
///
///
/// ============================================================================================================
///
/// Actions:
///
/// A body with an OWNED comm_status will be packed for exchange to create a ghost body on another rank when it
/// passes into one of this rank's shared regions, at which point the body is given a SHARED comm_status on this rank.
///
/// A body with a SHARED comm_status will become OWNED when it moves into the owned region on this rank.
/// A body with a SHARED comm_status will be removed when it moves into one of this rank's unowned regions.
///
/// A body with a GHOST comm_status will become OWNED when it moves into the owned region of this rank.
/// A body with a GHOST comm_status will be removed when it moves into the one of this rank's unowned regions.

class CH_DISTR_API ChDomainDistr {

public:
	ChDomainDistr(ChSystemDistr *sys);
	virtual ~ChDomainDistr();

	/// Defines the global space used for simulation. This space cannot be changed once set and
	/// needs to encompass all of the possible simulation region. If a body leaves the specified
	/// simulation domain, it may be removed from the simulation entirely.
	void SetSimDomain(double xlo, double xhi, double ylo, double yhi, double zlo, double zhi);
	
	/// Return the location of the specified body within this rank based on the data manager
	virtual distributed::COMM_STATUS GetBodyRegion(int index);
	/// Returns the location of the specified body within this rank based on the body-list

	virtual distributed::COMM_STATUS GetBodyRegion(std::shared_ptr<ChBody> body);

	/// Get the lower bounds of the global simulation domain
	ChVector<double> GetBoxLo() {return boxlo;}
	/// Get the upper bounds of the global simulation domain
	ChVector<double> GetBoxHi() {return boxhi;}

	/// Get the lower bounds of the local sub-domain
	ChVector<double> GetSubLo() {return sublo;}
	/// Get the upper bounds of the local sub-domain
	ChVector<double> GetSubHi() {return subhi;}

	// x = 0, y = 1, z = 2
	int GetLongAxis() {return long_axis;}

	/// Returns true if the domain has been set.
	bool IsSplit() {return split;}

	/// Prints basic information about the domain decomposition
	virtual void PrintDomain();

protected:
	ChSystemDistr *my_sys;

	int long_axis; ///< Index of the dimension of the longest edge of the global domain

	ChVector<double> boxlo; ///< Lower coordinates of the global domain
	ChVector<double> boxhi; ///< Upper coordinates of the global domain

	ChVector<double> sublo; ///< Lower coordinates of this subdomain
	ChVector<double> subhi; ///< Upper coordinates of this subdomain

	/// Calculates the borders of this subdomain based on the rank
	virtual void SplitDomain();
	bool split;

private:
	distributed::COMM_STATUS GetRegion(double pos);
};

} /* namespace chrono */

#endif