#ifndef BASE64_H
#define BASE64_H

extern "C" {
int             encode64(const char *_in, unsigned inlen,
                      char *_out, unsigned outmax, unsigned *outlen);


int             decode64(const char *in, unsigned inlen, char *out, unsigned *outlen);
}

/*
encode64(
    const char *_in, //Pointer to original string
    unsigned inlen,  //Length of original string
    char *_out, //Pointer to destination buffer
    unsigned outmax, //Length of destination buffer
    unsigned *outlen //Address of a variable that will be modified to take the new length of the buffer after encode is complete
);

int             decode64(
    const char *in,     // encoded string
    unsigned inlen,     // encoded string length
    char *out,          // pointer to location to write to
    unsigned *outlen    // Address of a variable that will be modified to take the new length of the buffer after encode is complete
);


*/

#endif