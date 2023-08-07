cmake_minimum_required(VERSION 3.14)

project(murlTests LANGUAGES CXX)

enable_testing()

# ---- Tests ----

add_executable(murl_test src/murl_test.cc)
target_link_libraries(murl_test PRIVATE LibMurl)
target_compile_features(murl_test PRIVATE cxx_std_20)

add_test(NAME murl_test COMMAND murl_test)
