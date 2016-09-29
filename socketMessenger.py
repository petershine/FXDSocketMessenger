import os
import sys
import socket

import argparse
import time, datetime
import logging
from multiprocessing import Process, Queue, Lock


global OPEN_SOCKET
OPEN_SOCKET = socket.socket()
OPEN_HOST = socket.gethostname()
OPEN_PORT = 12345

global CLIENT_CONNECTION, CLIENT_ADDRESS
CLIENT_CONNECTION = None
CLIENT_ADDRESS = None

global ACTIVE_CONNECTION
ACTIVE_CONNECTION = None

MESSAGE_BUFFER = 4096


def passiveReceivingWorker():

    while True:
        try:
            received = ACTIVE_CONNECTION.recv(MESSAGE_BUFFER)

            if len(received) > 0:
                print "\nReceived (", len(received), ") : ", received
                print "Message: ", ""
            else:
                break

        except KeyboardInterrupt, e:
            print e
            break

    print "ACTIVE CONNECTION: closed (Shutdown with Ctrl+C)"
    sys.exit(1)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    print "\n[[SOCKET MESSENGER]]\n"

    ap = argparse.ArgumentParser()
    ap.add_argument("-s", "--server", required=True, help="Decide to run as server")

    args = vars(ap.parse_args())

    #print "args: ", args, "\n"


    SHOULD_RUN_AS_SERVER = ((args['server'] == "True") | (args['server'] == "true") | (args['server'] == "TRUE"))
    #print "SHOULD_RUN_AS_SERVER: ", SHOULD_RUN_AS_SERVER

    if SHOULD_RUN_AS_SERVER:
        print "[[ RUN AS SERVER... ]]\n"
        OPEN_SOCKET.bind((OPEN_HOST, OPEN_PORT))
        OPEN_SOCKET.listen(5)  # Now wait for client connection.

        while True:
            try:
                print "Waiting for a client..."

                CLIENT_CONNECTION, CLIENT_ADDRESS = OPEN_SOCKET.accept()  # Establish connection with client.
                ACTIVE_CONNECTION = CLIENT_CONNECTION

                print "Got connection from:", CLIENT_ADDRESS
                ACTIVE_CONNECTION.send('Thank you for connecting')
                break

            except KeyboardInterrupt, e:
                print e
                break

    else:
        print "[[ RUN AS CLIENT... ]]\n"

        OPEN_SOCKET.connect((OPEN_HOST, OPEN_PORT))
        ACTIVE_CONNECTION = OPEN_SOCKET


    PASSIVE_RECEIVING_PROCESS = Process(target=passiveReceivingWorker, args=())
    PASSIVE_RECEIVING_PROCESS.daemon = True
    PASSIVE_RECEIVING_PROCESS.start()


    while True:
        try:
            message = raw_input("Message:\n")
            ACTIVE_CONNECTION.send(message)

        except KeyboardInterrupt, e:
            print e
            break


    print "\n\n[[ FINISH RUNNING... ]]\n"

    PASSIVE_RECEIVING_PROCESS.join()
    PASSIVE_RECEIVING_PROCESS.terminate()
    #print "DID JOIN?"


    if ACTIVE_CONNECTION is not None:
        ACTIVE_CONNECTION.close()

    if CLIENT_CONNECTION is not None:
        CLIENT_CONNECTION.close()  # Close the client connection from server

    OPEN_SOCKET.close()  # Close the socket when done
