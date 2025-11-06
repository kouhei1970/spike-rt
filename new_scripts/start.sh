sudo docker run --rm -it \
  -v $(pwd):$(pwd) -w $(pwd) \
  -u "$(id -u $USER):$(id -g $USER)" -v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
  ghcr.io/spike-rt/spike-rt:rich /bin/bash
