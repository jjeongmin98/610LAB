FROM debian
MAINTAINER Jeongmin
COPY . /
RUN apt-get update \
    && apt-get install -y libpcap-dev;
RUN apt-get install -y gcc

EXPOSE 80
EXPOSE 8080


