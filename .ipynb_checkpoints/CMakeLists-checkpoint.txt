# ########################################################################################
# Use the code in this section for standalone projects
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(RPC)
if(NOT TARGET Garfield::Garfield)
  find_package(Garfield REQUIRED)
endif()
# ########################################################################################

# ---Build executable------------------------------------------------------------
add_executable(Gif RPC_Gif.C)
target_link_libraries(Gif Garfield::Garfield)

add_executable(Gas GasTableGenerator.C)
target_link_libraries(Gas Garfield::Garfield)

add_executable(Weight Weight.C)
target_link_libraries(Weight Garfield::Garfield)

add_executable(Field FieldLines.C)
target_link_libraries(Field Garfield::Garfield)

add_executable(Final RPC_DoubleGap_Complete.C)
target_link_libraries(Final Garfield::Garfield)

# ---Copy all files locally to the build directory-------------------------------
foreach(_file cms_rpc_95.2_4.5_0.3_25-40kV.gas)
  configure_file(${_file} ${_file} COPYONLY)
endforeach()
