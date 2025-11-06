if [ "$1" = "" ]; then
	    echo "no argument"
	    exit 1
fi
appname=$1
if [ ! -e "workspace/${appname}" ]; then
	  echo "Such a '${appname}' dosen't exit."
	  exit 1
fi

sudo scripts/deploy-dfu.sh workspace/$appname/asp.bin
