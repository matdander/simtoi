/*
 * CTaskFactory.h
 *
 *  Created on: Feb 10, 2013
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

#ifndef CTASKFACTORY_H_
#define CTASKFACTORY_H_

#include <map>
#include <memory>
#include <vector>

using namespace std;

class CTask;
class CWorkerThread;
typedef shared_ptr<CWorkerThread> CWorkerPtr;

typedef shared_ptr<CTask> (*CreateTaskFn)(CWorkerThread * WorkerThread);


class CTaskFactory {
private:
	map<string, CreateTaskFn> mFactory;

	CTaskFactory();

public:
	virtual ~CTaskFactory();

public:
	shared_ptr<CTask> CreateWorker(string WorkerID, CWorkerThread * WorkerThread);

	static CTaskFactory Instance();

	void Register(string WorkerID, CreateTaskFn CreateFunction);

	vector<string> GetWorkerList();
};

#endif /* CTASKFACTORY_H_ */
