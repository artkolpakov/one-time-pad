The one-time pad (OTP) is an encryption technique that cannot be cracked, but requires the use of a single-use pre-shared key that is not smaller 
than the message being sent. In this technique, a plaintext is paired with a random secret key (also referred to as a one-time pad). Then, each bit 
or character of the plaintext is encrypted by combining it with the corresponding bit or character from the pad using modular addition.

---------------------------------------------

enc_server:

This program is the encryption server that runs in the background, listening on a particular port/socket. The server first verifies that 
the connection to enc_server is coming from enc_client, then the child (version of enc_server supports up to five concurrent socket connections running at the same time) 
receives plaintext and a key from enc_client via the connected socket. Using the obtained key, enc_server encrypts the plaintext, and then the enc_server 
child writes the encrypted data back to the enc_client process to which it is connected via the same socket.

Use this syntax for enc_server: enc_server <listening_port>

---------------------------------------------

enc_client:

This program connects to enc_server, and asks it to perform a one-time pad style encryption. If enc_client receives key or plaintext files with any bad characters in them (the allowed 
characters are the 26 capital letters, and the space character), or the key file is shorter than the plaintext, 
then it terminates, sending appropriate error text to stderr, and setting the exit value to 1. If all the data enc_client obtains from the key and plaintext files is valid, then enc_client starts sending the key and 
plaintext for encryption to enc_server. After enc_server does the encryption and sends the encrypted data back, enc_client receives the ciphertext from enc_server and writes it to standard output.

Use this syntax for enc_client: enc_client <plaintextFile> <keyFile> <port>

where port is the port that enc_client should attempt to connect to enc_server on, plaintextFile is a file that contains plaintext to get encrypted (I provided an example one), and keyFile is a file that contains the key.

---------------------------------------------

dec_server & dec_client:

Work exactly like enc_server and enc_client, except for the fact that dec_server decrypts the ciphertext passed to it using the passed ciphertext and key, and therefore returns the plaintext back to dec_client.

syntax: dec_server <listening_port>, dec_client <plaintextFile> <keyFile> <port>

---------------------------------------------

keygen:

Generates a random key (which is then used for encryption/decryption) of the specified length.

Use this syntax for keygen: keygen <keylength>

---------------------------------------------

compileall script:

a shell script that compiles all the needed files for the system to work (enc_server.c, enc_client.c, dec_server.c, dec_client.c, and keygen.c). You will need to 
execute chmod +x compileall to prepare the script to run :). After running the script, you may use enc_server, enc_client, dec_server, dec_client, and keygen according to the described above syntax.

---------------------------------------------

plaintext:

an example file that you can use as an example of plain text.

P.S. the implementation details are documented directly in the .c files.
