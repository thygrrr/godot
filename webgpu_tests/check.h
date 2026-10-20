#pragma once

#include <cstdio>
#include <cstdlib>

// Always-on test assertion; assert() disappears under NDEBUG.
#define CHECK(m_cond) \
	((m_cond) ? (void)0 : (std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #m_cond), std::exit(1)))
