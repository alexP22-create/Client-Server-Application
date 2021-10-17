server:
	gcc server.c -o server -g

subscriber:
	gcc subscriber.c -o subscriber -lm -g

clean:
	rm server subscriber
