FROM nvidia/cuda:9.1-runtime

ADD mt /mt

WORKDIR /mt

EXPOSE 17005

CMD ["./bin/mtd", "-H", "::"]