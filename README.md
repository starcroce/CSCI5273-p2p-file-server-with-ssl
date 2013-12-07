To run server:
./server_PFS 9000 server.key server.crt cacert.pem

To run client:
./client_PFS A 127.0.1.1 9000 clientA_priv.key clientA.crt cacert.pem
./client_PFS B 127.0.1.1 9000 clientB_priv.key clientB.crt cacert.pem
./client_PFS C 127.0.1.1 9000 clientC_priv.key clientC.crt cacert.pem
