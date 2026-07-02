# MSVC Windows Toolchain Configuration for CMake
# This file ensures proper Windows SDK configuration

# Detect Windows SDK path from Visual Studio installation
if(MSVC)
	# Get Visual Studio installation directory
	get_filename_component(VS_DIR "$ENV{VSINSTALLDIR}" ABSOLUTE)

	# Common Windows SDK paths
	if(EXISTS "C:/Program Files (x86)/Windows Kits/10")
		set(WINDOWS_SDK_ROOT "C:/Program Files (x86)/Windows Kits/10")
	elseif(EXISTS "C:/Program Files/Windows Kits/10")
		set(WINDOWS_SDK_ROOT "C:/Program Files/Windows Kits/10")
	else()
		# Try to find from registry or environment
		if(DEFINED ENV{WindowsSDKDir})
			set(WINDOWS_SDK_ROOT "$ENV{WindowsSDKDir}")
		endif()
	endif()

	if(WINDOWS_SDK_ROOT)
		# Add library directories
		link_directories("${WINDOWS_SDK_ROOT}/Lib/${WINDOWS_SDK_VERSION}/um/${CMAKE_SYSTEM_PROCESSOR}")
		link_directories("${WINDOWS_SDK_ROOT}/Lib/${WINDOWS_SDK_VERSION}/ucrt/${CMAKE_SYSTEM_PROCESSOR}")
	endif()

	# Ensure proper linking flags
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")

	# Set proper runtime library
	foreach(flag_var
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
		if(${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MDd" ${flag_var} "${${flag_var}}")
		endif()
	endforeach()
endif()
