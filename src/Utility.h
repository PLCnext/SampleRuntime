 /******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  Utility.h
 *
 *  Created on: Jul 12, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#ifndef UTILITY_H_
#define UTILITY_H_

#define WAIT100ms 	usleep(100000);		// 100ms
#define WAIT1s 		usleep(1000000);	// 1s
#define WAIT10s 	usleep(10000000);	// 10s

typedef void * (*THREADFUNCPTR)(void *);

#endif /* UTILITY_H_ */
