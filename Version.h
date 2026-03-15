#pragma once

// -----------------------------
// Version numbers
// -----------------------------

#define NHM_VERSION_MAJOR 1
#define NHM_VERSION_MINOR 2
#define NHM_VERSION_PATCH 0

// Numeric version for resource compiler
#define NHM_VERSION_NUMERIC NHM_VERSION_MAJOR,NHM_VERSION_MINOR,NHM_VERSION_PATCH,0

// Version for Rainmeter
#define NHM_RAINMETER_VERSION (NHM_VERSION_MAJOR * 1000 + NHM_VERSION_MINOR + NHM_VERSION_PATCH)

// String helpers
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define NHM_VERSION_STRING STR(NHM_VERSION_MAJOR) "." STR(NHM_VERSION_MINOR) "." STR(NHM_VERSION_PATCH)

// -----------------------------
// Plugin metadata
// -----------------------------

#define NHM_AUTHOR "Kurou"
#define NHM_INFORMATION "Native number library for Rainmeter"
#define NHM_COPYRIGHT "\x00A9 2026"
#define NHM_COMPANY "Kurou Productions"

// -----------------------------
// Plugin name
// -----------------------------

#define NHM_PRODUCT_NAME "NativeHardwareMonitor"
