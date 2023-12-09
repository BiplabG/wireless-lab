#include <Crypto.h>
#include <AES.h>
#include <string.h>

//key[16] cotain 16 byte key(128 bit) for encryption
byte key[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
//plaintext[16] contain the text we need to encrypt
// byte plaintext[16]={0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
//cypher[16] stores the encrypted text
// byte cypher[16];
//decryptedtext[16] stores decrypted text after decryption
byte decryptedtext[16];
//creating an object of AES128 class
AES128 aes128;

void printByteSerial(byte *text, size_t length){
  for(int j=0;j<length;j++){
      Serial.write(text[j]);
   }
  Serial.write("\n");
}

void printTextSerial(char *text, size_t length){
  for(int j=0;j<length;j++){
      Serial.write(text[j]);
   }
   Serial.write("\n");
}

char encryptedText[128];
char decryptedText[128];

void encryptWhole(char *plain, size_t length){
  Serial.println("Before Encryption\n");
  printTextSerial(plain, length);

  int byte_len;
  if (length % 16 == 0){
    byte_len = length;
  } else {
    byte_len = (length / 16) * 16 + 16;
  }
  
  byte plainbyte[byte_len];

  for (int i=0; i < byte_len-1; i++){
    if (i >= length){
      plainbyte[i] = 0x23;
    }
    else {
      plainbyte[i] = plain[i];
    }
  }
  byte cypher[byte_len];

  byte *cp;
  byte *pbp;
  cp = cypher;
  pbp = plainbyte;
  for (int j=0; j<byte_len; j=j+16){
    aes128.encryptBlock(cp + j, pbp + j);
  }

  Serial.println("After Encryption");
  printByteSerial(cypher, sizeof(cypher));

  // char encryptedText[byte_len];
  for (int i=0; i < byte_len; i++){
      encryptedText[i] = cypher[i];
  }
  Serial.println(encryptedText);
}

void decryptWhole(char *encryptedText, size_t length){
  int byte_len;
  if (length % 16 == 0){
    byte_len = length;
  } else {
    byte_len = (length / 16) * 16 + 16;
  }

  byte cypher[byte_len];
  for (int i=0; i < byte_len; i++){
    cypher[i] = encryptedText[i];
  }

  byte decryptedByte[byte_len];
  byte *dcp;
  byte *dpbp;
  dcp = cypher;
  dpbp = decryptedByte;
  for (int j=0; j<byte_len; j=j+16){
    aes128.decryptBlock(dpbp+j, dcp+j);
  }

  // char decryptedText[byte_len];
  for (int i=0; i < byte_len-1; i++){
      decryptedText[i] = decryptedByte[i];
  }
  Serial.println("Final decrypted text");
  Serial.println(decryptedText);
}


void setup() {
  Serial.begin(9600);
  aes128.setKey(key,16);// Setting Key for AES
  
  char plainText[] = "This is the text to encrypt in total. Can you do this as much as you want bro in life?";
  encryptWhole(plainText, strlen(plainText));
  decryptWhole(encryptedText, strlen(plainText));
  Serial.println(decryptedText);
}

void loop() {
  // put your main code here, to run repeatedly:

}