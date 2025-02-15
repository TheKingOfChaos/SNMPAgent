#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Disable floating point support to save space
#define UNITY_EXCLUDE_FLOAT
#define UNITY_EXCLUDE_DOUBLE
#define UNITY_EXCLUDE_FLOAT_PRINT

// Use standard output functions
#define UNITY_OUTPUT_CHAR(c) putchar(c)
#define UNITY_OUTPUT_START() (void)0
#define UNITY_OUTPUT_FLUSH() fflush(stdout)
#define UNITY_OUTPUT_COMPLETE() (void)0

#endif // UNITY_CONFIG_H
