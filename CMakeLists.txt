cmake_minimum_required(VERSION 3.15)
project(improg)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
  set(improg_common_flags /Wall /WX /wd4820 /wd4668 /wd5045)
else()
  set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g3")
  set(CMAKE_C_FLAGS_RELEASE "-Os")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g3")
  set(CMAKE_CXX_FLAGS_RELEASE "-Os")

  set(improg_common_flags -flto -pedantic -Wall -Wextra -Wundef -Werror)
  set(improg_link_flags -flto)

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND improg_common_flags
         -Weverything
         -Wno-padded
         -Wno-format-nonliteral
         -Wno-declaration-after-statement
         -Wno-covered-switch-default)
    if (CMAKE_HOST_APPLE)
      list(APPEND improg_common_flags -Wno-poison-system-directories)
    endif()
  else()
    list(APPEND improg_common_flags
         -Wconversion
         -Wshadow
         -Wfloat-equal
         -Wsign-conversion
         -Wswitch-enum
         -Wswitch-default)
  endif()
endif()

# improg lib
add_library(improg STATIC improg.c remprog.c)
target_include_directories(improg PUBLIC include)
target_compile_options(improg PRIVATE ${improg_common_flags})

# improg demo
add_executable(improg-demo examples/improg-demo.c)
target_compile_options(improg-demo PRIVATE ${improg_common_flags})
target_link_options(improg-demo PRIVATE ${improg_link_flags})
target_link_libraries(improg-demo improg)
if (NOT MSVC)
  target_link_libraries(improg-demo m)
endif()
