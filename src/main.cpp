#define JANPATCH_STREAM FILE
// #define JANPATCH_DEBUG(...) Serial.printf(__VA_ARGS__)
// #define JANPATCH_ERROR(...) Serial.printf(__VA_ARGS__)

#include <Arduino.h>
#include "janpatch.h"
#include "SPIFFS.h"
#include <mbedtls/sha1.h>

String calcSHA1(String filepath);

janpatch_ctx ctx = {
    // fread/fwrite buffers for every file, minimum size is 1 byte
    // when you run on an embedded system with block size flash, set it to the size of a block for best performance
    { (unsigned char*)malloc(1024), 1024 },
    { (unsigned char*)malloc(1024), 1024 },
    { (unsigned char*)malloc(1024), 1024 },

    // define functions which can perform basic IO
    // on POSIX, use:
    &fread,
    &fwrite,
    &fseek,
    &ftell          // NOTE: passing ftell is optional, and only required when you need progress reports
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPIFFS.begin(true);

  Serial.println("List dir");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print("FILE: ");
    Serial.print(file.name()); Serial.print("\t\t");
    Serial.print(calcSHA1(file.name())); Serial.print("\t");
    Serial.println(file.size());
    file = root.openNextFile();
  }
  file.close();

  Serial.println("Patching new firmware...");
  unsigned long startTime = millis();

  JANPATCH_STREAM *source = fopen("/spiffs/firmware_old.bin", "rb");
  JANPATCH_STREAM *patch  = fopen("/spiffs/patch.bin", "rb");
  JANPATCH_STREAM *target = fopen("/spiffs/firmware_patched.bin", "wb");

  if (source == NULL || patch == NULL || target == NULL) {
    Serial.println("Error occurred!");
    return;
  }

  int result = janpatch(ctx, source, patch, target);
  if (result != 0) {
    Serial.println("Patch failed!");
    return;
  }

  fclose(source);
  fclose(patch);
  fclose(target);

  Serial.println(millis() - startTime);
  Serial.println("done");
  Serial.println("List dir");
  root = SPIFFS.open("/");
  file = root.openNextFile();
  while (file) {
    Serial.print("FILE: ");
    Serial.print(file.name()); Serial.print("\t\t");
    Serial.print(calcSHA1(file.name())); Serial.print("\t");
    Serial.println(file.size());
    file = root.openNextFile();
  }
  file.close();
}

void loop() {
  // put your main code here, to run repeatedly:
}

String calcSHA1(String filepath) {
  // Open file
  File f = SPIFFS.open(filepath, FILE_READ);
  if (!f) {
    return "";
  }

  int fileSize = f.size();

  // Setup SHA1
  uint8_t output[20];
  mbedtls_sha1_context ctx;

  mbedtls_sha1_init(&ctx);
  mbedtls_sha1_starts_ret(&ctx);

  // Checking file size
  int bufferSize = 1024;
  uint8_t sha1_buf[bufferSize];
  int iteration = (fileSize / bufferSize) + 1;

  // Start streaming data to SHA1 engine
  for (int i = 0; i < iteration; i++) {
    int bytesRead = f.readBytes((char*)sha1_buf, bufferSize);
    mbedtls_sha1_update_ret(&ctx, sha1_buf, bytesRead);
    yield();
  }
  mbedtls_sha1_finish_ret(&ctx, output);
  mbedtls_sha1_free(&ctx);

  // Convert SHA1 output into hexstring
  char sha1String[40];
  sprintf(sha1String, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
          output[0], output[1], output[2], output[3], output[4],
          output[5], output[6], output[7], output[8], output[9],
          output[10], output[11], output[12], output[13], output[14],
          output[15], output[16], output[17], output[18], output[19]
          );
  
  return String(sha1String);
}