/*
 * Copyright (C) 2013 Antony Woods <antony@teamwoods.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Antony Woods <antony@teamwoods.org>
 */

#pragma once

namespace entityx {
namespace help {

class Timer
{
public:
	Timer();
	~Timer();

	void restart();
	double elapsed();
};

} // namespace help
} // namespace entityx