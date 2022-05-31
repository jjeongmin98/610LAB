FROM ubuntu:18.04
LABEL maintainer="Jeongmin Kim<eodjr05@naver.com>"
#install libcap
RUN apt-get update \
    && apt-get install libcap-dev; exit 0
EXPOSE 80
