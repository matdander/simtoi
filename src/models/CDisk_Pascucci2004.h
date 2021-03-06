/*
 * CDisk_Pascucci2004.h
 *
 *  Created on: Dec 28, 2012
 *      Author: bkloppen
 *
 *  A class implementing a disk model consisting of a series of partially
 *  opaque concentric rings. Each ring has a scale height transparency which
 *  decays from the midplane transparency to 1 (completely transparent)
 *  following a Gaussian distribution.
 *
 *  The model has the following parameters:
 *  	r_in : the inner radius (>= 0)
 *  	r_out: the outer radius (> 0)
 *  	midplane_trans : the transparency of the midplane (0...1), normally 0 (completely opaque)
 *  	beta : the Gaussian decay coefficient in the z-direction (> 0)
 *  	n_rings: the number of concentric rings between r_in and r_out (> 0)
 *
 *  NOTE: If beta is set outside of their specified bounds, the transparency
 *  in the corresponding direction will be set to zero (i.e. 100% opaque).
 *
 *  WARNING: This class explicitly overrides the default parameters in CModelDisk::InitMembers
 */

#ifndef CDISK_PASCUCCI2004_H_
#define CDISK_PASCUCCI2004_H_

#include "CDensityDisk.h"

class CDisk_Pascucci2004: public CDensityDisk
{

public:
	CDisk_Pascucci2004();
	virtual ~CDisk_Pascucci2004();

	static shared_ptr<CModel> Create();

	virtual string GetID() { return "disk_pascucci2004"; };
};

#endif /* CDISK_PASCUCCI2004_H_ */
