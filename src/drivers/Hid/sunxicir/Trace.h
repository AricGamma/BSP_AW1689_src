
// {CA0E4400-4AE1-45F6-A380-5EFC31CFB61D}
#define WPP_CONTROL_GUIDS	\
		WPP_DEFINE_CONTROL_GUID( \
			SunxicirTraceGuid, (CA0E4400, 4AE1, 45F6, A380, 5EFC31CFB61D),	\
																			\
			WPP_DEFINE_BIT(SUNXICIR_ALL_INFO)                               \
			WPP_DEFINE_BIT(TRACE_DRIVER)                                    \
			WPP_DEFINE_BIT(TRACE_DEVICE)                                    \
			WPP_DEFINE_BIT(TRACE_QUEUE)                                     \
		)


#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

#define DEBUG_LEVEL_FATAL       0
#define DEBUG_LEVEL_ERROR       1
#define DEBUG_LEVEL_WARNING     2
#define DEBUG_LEVEL_INFORMATION 3
#define DEBUG_LEVEL_TERSE       4
#define DEBUG_LEVEL_VERBOSE     5

extern unsigned long g_DebugLevel;

#define DebugPrint(_LEVEL_, _FORMAT_, ...) \
if (g_DebugLevel >= _LEVEL_) \
{ \
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "SunxiCIR:%s %s (%d): ", __TIME__, __FUNCTION__, __LINE__); \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, _FORMAT_, __VA_ARGS__); \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n"); \
} 

#define DbgPrint_E(...) DebugPrint(DEBUG_LEVEL_ERROR, __VA_ARGS__)
#define DbgPrint_W(...) DebugPrint(DEBUG_LEVEL_WARNING, __VA_ARGS__)
#define DbgPrint_I(...) DebugPrint(DEBUG_LEVEL_INFORMATION, __VA_ARGS__)
#define DbgPrint_T(...) DebugPrint(DEBUG_LEVEL_TERSE, __VA_ARGS__)
#define DbgPrint_V(...) DebugPrint(DEBUG_LEVEL_VERBOSE, __VA_ARGS__)

#define FunctionEnter() DbgPrint_V("Entered.")
#define FunctionExit(_STATUS_) DbgPrint_V("Exit with 0x%lx.", _STATUS_)