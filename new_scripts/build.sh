#!/bin/sh
BUILD_DIR=${BUILD_DIR:-build}
JOB_NUM=${JOB_NUM:-$(nproc)}

if [ "$1" = "" ]; then
	    echo "no argument"
	    exit 1
fi
appname=$1
if [ ! -e "workspace/${appname}" ]; then
	  echo "Such a '${appname}' dosen't exit."
	  exit 1
fi

# cd spike-rt
mkdir -p $BUILD_DIR
cd $BUILD_DIR

if [ ! -e "obj-primehub_kernel" ]; then
	  mkdir -p obj-primehub_kernel
	  (cd obj-primehub_kernel && ../../asp3/configure.rb -T primehub_gcc -f -m ../../common/kernel.mk)
fi

if [ ! -e "obj-primehub_${appname}" ]; then
	  mkdir -p obj-primehub_$appname
	  (cd obj-primehub_$appname && ../../asp3/configure.rb -T primehub_gcc -L ../obj-primehub_kernel -a ../../workspace/$appname/ -A $appname -m ../../common/app.mk)
fi

# Currently, build libpybricks.a, first of all.
# libkernel.a itself does not depend on libpybricks.a, 
# but some codes in libkernel.a depend on header files which is cloned by `make libpybricks.a`. 
(cd obj-primehub_kernel && make libpybricks.a -j $JOB_NUM && make libkernel.a -j $JOB_NUM)

(cd obj-primehub_$appname && make -j $JOB_NUM && make asp.bin -j $JOB_NUM)

(cd obj-primehub_$appname && cp asp.bin ../../workspace/$appname/asp.bin)

cd ..
