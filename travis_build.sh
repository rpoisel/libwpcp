set -e

if [[ $1 == coverage ]]; then
  mkdir build
  cd build
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DWPCP_BUILD_LIBWEBSOCKETS=ON -DWPCP_GENERATE_COVERAGE=ON $2
  cmake --build .
  cmake --build . --target coverage_reset
  cmake --build . --target test
  cmake --build . --target coverage_generate
elif [[ $1 == release ]]; then
  mkdir build
  cd build
  cmake .. -DWPCP_BUILD_LIBWEBSOCKETS=ON $2
  cmake --build .
  cmake --build . --target test
elif [[ $1 == lxc ]]; then
  CONTAINER_NAME=libwpcp-build-box-$3-$4-`date +%s`
  sudo apt-get -qq update
  sudo apt-get -t trusty-backports install -y lxc lxc-templates
  sudo lxc-create -n $CONTAINER_NAME -t ubuntu-cloud -- -a $4 --release $3
  sudo lxc-start -n $CONTAINER_NAME
  sudo lxc-attach -n $CONTAINER_NAME -- mkdir /git_build
  sudo lxc-attach -n $CONTAINER_NAME -- mkdir /git_source
  sudo lxc-stop -n $CONTAINER_NAME
  sudo bash -c "echo \"lxc.mount.entry = $PWD git_source none ro,bind 0.0\" >> /var/lib/lxc/$CONTAINER_NAME/config"
  sudo lxc-start -n $CONTAINER_NAME
  sudo lxc-attach -n $CONTAINER_NAME -- bash -c 'while ! ping -c 1 ubuntu.com; do sleep 1; done'
  sudo lxc-attach -n $CONTAINER_NAME -- apt-get -qq update
  sudo lxc-attach -n $CONTAINER_NAME -- apt-get install -y cmake check git libssl-dev make
  sudo lxc-attach -n $CONTAINER_NAME -- cmake -B/git_build -H/git_source -DWPCP_BUILD_LIBWEBSOCKETS=ON $2
  sudo lxc-attach -n $CONTAINER_NAME -- cmake --build /git_build
  sudo lxc-attach -n $CONTAINER_NAME -- cmake --build /git_build --target test
  sudo lxc-stop -n $CONTAINER_NAME
  sudo lxc-destroy -n $CONTAINER_NAME
fi
