/*
 * Architecture - specific definitions 
 */

#include "config.h"

#define NUMSPACES 4

const int moreIndent = NUMSPACES;
const int lessIndent = -NUMSPACES;

const unsigned MAXTASKS = 2; 
const unsigned N[] = {
	6,
	2,
};
const unsigned NUMPARAMS[] = {
	2,
	2,
};

const unsigned PARAMS[2][2] = {
	{12, 4},
	{8, 4},
};

// const unsigned NUMBANKS = 8;

const unsigned TASK[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	1,
	1,
};

const unsigned OFFSET[] = {
	0,
	6,
};

// const unsigned BANKLATENCY = 1;

// const unsigned DELTAS[8][8] = 
// {
// 	{1, 4, 4, 4, 4, 4, 4, 4},
// 	{4, 1, 4, 4, 4, 4, 4, 4},
// 	{4, 4, 1, 4, 4, 4, 4, 4},
// 	{4, 4, 4, 1, 4, 4, 4, 4},
// 	{4, 4, 4, 4, 1, 4, 4, 4},
// 	{4, 4, 4, 4, 4, 1, 4, 4},
// 	{4, 4, 4, 4, 4, 4, 1, 4},
// 	{4, 4, 4, 4, 4, 4, 4, 1},
// };