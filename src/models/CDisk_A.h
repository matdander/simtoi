/*
 * CDisk_A.h
 *
 *  Created on: Feb 24, 2012
 *      Author: bkloppenborg
 */
 
 /* 
 * Copyright (c) 2012 Brian Kloppenborg
 *
 * If you use this software as part of a scientific publication, please cite as:
 *
 * Kloppenborg, B.; Baron, F. (2012), "SIMTOI: The SImulation and Modeling 
 * Tool for Optical Interferometry" (Version X). 
 * Available from  <https://github.com/bkloppenborg/simtoi>.
 *
 * This file is part of the SImulation and Modeling Tool for Optical 
 * Interferometry (SIMTOI).
 * 
 * SIMTOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * as published by the Free Software Foundation version 3.
 * 
 * SIMTOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with SIMTOI.  If not, see <http://www.gnu.org/licenses/>.
 */
 
 /*
 *  Disk Model A:
 *  This model modifies the edges of the simple CModelDisk to extend radially outward
 *  with an exponentially decaying height:
 *
 *  	h(r) = h0 * exp( -(r-r0)/h_r )
 *
 *  where h0 is the initial height, r0 is the inner radius of the decaying region and
 *  h_r is a decay factor (akin to scale height)
 *
 */

#ifndef CDISK_A_H_
#define CDISK_A_H_

#include "CCylinder.h"

class CDisk_A: public CCylinder
{
public:
	CDisk_A();
	virtual ~CDisk_A();

	static shared_ptr<CModel> Create();

	virtual string GetID() { return "disk_a"; };
	double GetRadius(double half_height, double h, double dh, double rim_radius);
};

#endif /* CDISK_A_H_ */
