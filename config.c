/*
 * Architecture - specific definitions 
 */

#include "config.h"

#define NUMSPACES 4

const int moreIndent = NUMSPACES;
const int lessIndent = -NUMSPACES;

const unsigned MAXTASKS = 3; 
const unsigned N[] = {
	6,
	2,
	2
};
const unsigned NUMPARAMS[] = {
	2,
	2,
	2
};

const unsigned PARAMS[2][2] = {
	{8, 4},
	{8, 4},
};
