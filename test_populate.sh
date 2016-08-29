#!/bin/bash

(
	for i in {1..2499}
	do
		printf "set item%d 0 0 %d noreply\r\n%s\n\r\n" $i $(cat main.c | wc -c) "$(cat main.c)" | nc localhost 11211
	done
) &

(
	for i in {2500..4999}
	do
		printf "set item%d 0 0 %d noreply\r\n%s\n\r\n" $i $(cat LICENSE | wc -c) "$(cat LICENSE)" | nc localhost 11211
	done
) &

(
	for i in {5000..7499}
	do
		printf "set item%d 0 0 %d noreply\r\n%s\n\r\n" $i $(cat README.md | wc -c) "$(cat README.md)" | nc localhost 11211
	done
) &

(
	for i in {7500..9999}
	do
		printf "set item%d 0 0 %d noreply\r\n%s\r\n" $i $(cat CMakeLists.txt | wc -c) "$(cat CMakeLists.txt)" | nc localhost 11211
	done
) &

wait
