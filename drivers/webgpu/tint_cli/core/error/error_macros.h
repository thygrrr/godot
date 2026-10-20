// Minimal shim for Godot's error macros — standalone CLI builds only. Messages are C strings.
#pragma once

#include <cstdio>

#define ERR_PRINT(m_msg) std::fprintf(stderr, "ERROR: %s\n", (const char *)(m_msg))
#define WARN_PRINT(m_msg) std::fprintf(stderr, "WARNING: %s\n", (const char *)(m_msg))
#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg) \
	if (m_cond) {                                    \
		ERR_PRINT(m_msg);                            \
		return m_retval;                             \
	} else                                           \
		((void)0)
