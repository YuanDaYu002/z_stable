svn up

v=`svn log|sed -n '2,2p'|awk '{print $1}'`

m=`date +"%m"`

version="V1."${m}"."${v}

echo "LIB_VERSION=\\\"$version\\\"" >version
