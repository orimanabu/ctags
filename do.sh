#!/bin/bash

if [ x"$#" != x"1" ]; then
	echo "$0 arg"
	exit 1
fi
arg=$1; shift

builddir=./build-wasm

case ${arg} in
step0|0|setup)
 	#git clone git@github.com:orimanabu/ctags.git -b wasm ctags-wasm
 	#cd ctags-wasm/
 	git clone https://github.com/emscripten-core/emsdk.git
 	cd emsdk/
 	./emsdk install latest
 	./emsdk activate latest
 	source emsdk_env.sh
 	which emcc emconfigure
	;;
step1|1|configure)
	./autogen.sh

	mkdir ${builddir} && cd ${builddir}

	emconfigure ../configure \
	  --prefix=/Users/ori/ctags-wasm \
	  --disable-seccomp \
	  --disable-xml \
	  --disable-json \
	  --disable-yaml \
	  --disable-pcre2 \
	  --disable-iconv \
	  LDFLAGS="-s NODERAWFS=1" 2>&1 | tee log.configure
	;;
step2|2|packcc)
	cd ${builddir} && cc -fsigned-char -DPCC_USE_SYSTEM_STRNLEN -o packcc ../misc/packcc/src/packcc.c
	;;
step3|3|node)
	cd ${builddir} && emmake make 2>&1 | tee log.node
	;;
step4|4|wasm)
	cd ${builddir} && make wasm 2>&1 | tee log.wasm
	;;
test)
	cmd="file build-wasm/ctags.wasm"
	echo "=> ${cmd}"
	${cmd}
	echo

	cmd="node --print-wasm-code build-wasm/ctags --sort=no -f - main/main.c"
	echo "=> ${cmd}"
	${cmd} | grep -B1 -A10 '^name: ctags'
	echo

	cmd="node build-wasm/ctags --sort=no -f - main/kind.c"
	echo "=> ${cmd}"
	${cmd} | head -n 5
	;;
demo)
	cd ${builddir} && node wasm-demo.js
	;;
diff)
	git diff origin/oneshot+objdump
	;;
*)
	echo "Unknown arg: ${arg}"
	exit 1
	;;
esac
