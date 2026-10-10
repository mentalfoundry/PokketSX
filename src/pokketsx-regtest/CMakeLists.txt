add_executable(pokketsx-regtest
  regtest_host.cpp
)

target_link_libraries(pokketsx-regtest PRIVATE core common scmversion)

add_core_resources(pokketsx-regtest)