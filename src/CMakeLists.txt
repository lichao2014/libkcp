aux_source_directory(. SRC)
file(GLOB INC *.h)

add_library(kcp ${SRC} ${INC})