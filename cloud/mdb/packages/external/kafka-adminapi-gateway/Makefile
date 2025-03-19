
clean:
	docker run --rm -v "$(shell pwd)":/usr/src/mymaven -w /usr/src/mymaven maven:3.8-openjdk-8 mvn -B clean

package:
	docker run --rm -v "$(shell pwd)":/usr/src/mymaven -w /usr/src/mymaven maven:3.8-openjdk-8 mvn -B -s settings.xml package

push:
	ya upload --sandbox --ttl=inf target/kafka-adminapi-gateway-1-jar-with-dependencies.jar
