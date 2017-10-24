MKDIR_P := mkdir -p
OUT_DIR := build
.PHONY: directories all clean
all: directories hookmake
directories: ${OUT_DIR}
${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}
hookmake: libminm.c libintercept.c
	gcc libminm.c -o ${OUT_DIR}/libminm.so -fPIC -shared -ldl -D_GNU_SOURCE
	gcc libintercept.c -o ${OUT_DIR}/libintercept.so -fPIC -shared -ldl -D_GNU_SOURCE
